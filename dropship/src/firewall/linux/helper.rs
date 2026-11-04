use std::io::{BufRead, Write};
use std::os::fd::AsRawFd;
use std::path::{Path, PathBuf};
use std::process::{Command, Output, Stdio};

use super::netscope;
use super::protocol::{Request, Response};
use super::ruleset::{self, Ruleset};

/// The process netscope's scope targets, and what dropship looks for in its own
/// game detection. The block follows this name, not the whole user.
pub(crate) const GAME_PROCESS: &str = "Overwatch.exe";

const LOCK_PATH: &str = "/run/dropship-firewall.lock";
const STATE_DIR: &str = "/var/lib/dropship";
const STATE_PATH: &str = "/var/lib/dropship/blocked.nft";
const UNIT_PATH: &str = "/etc/systemd/system/dropship-firewall.service";
const UNIT_NAME: &str = "dropship-firewall.service";
const UNIT: &str = include_str!("../../../assets/linux/dropship-firewall.service");

// one cgroup under the user manager holds the game and the rules match it, so
// the block follows the game instead of every socket the user owns. it lives
// in the user's own subtree because a cgroup the user owns is the only one the
// kernel lets them join without root
pub(crate) const CGROUP_NAME: &str = "dropship-game";

pub(crate) fn cgroup_rel_path(uid: u32) -> String {
    format!("user.slice/user-{uid}.slice/user@{uid}.service/{CGROUP_NAME}")
}

pub(crate) fn cgroup_level(rel_path: &str) -> u32 {
    rel_path.split('/').count() as u32
}

pub(crate) fn cgroup_dir(uid: u32) -> PathBuf {
    PathBuf::from("/sys/fs/cgroup").join(cgroup_rel_path(uid))
}

const NFT_PATHS: [&str; 4] = ["/usr/sbin/nft", "/usr/bin/nft", "/sbin/nft", "/bin/nft"];
const SYSTEMCTL_PATHS: [&str; 2] = ["/usr/bin/systemctl", "/bin/systemctl"];

pub(crate) fn run() -> i32 {
    let lock = match std::fs::OpenOptions::new()
        .create(true)
        .write(true)
        .open(LOCK_PATH)
    {
        Ok(file) => file,
        Err(e) => {
            write_response(&failure(format!("failed to open the firewall lock: {e}")));
            return 1;
        }
    };

    if unsafe { libc::flock(lock.as_raw_fd(), libc::LOCK_EX | libc::LOCK_NB) } != 0 {
        write_response(&failure(
            "another dropship instance is already managing the firewall",
        ));
        return 1;
    }

    let _lock = lock;

    let Some(nft) = resolve("DROPSHIP_NFT_BIN", &NFT_PATHS) else {
        write_response(&failure("nft was not found; install nftables"));
        return 1;
    };

    let mut helper = Helper {
        nft,
        systemctl: resolve("DROPSHIP_SYSTEMCTL_BIN", &SYSTEMCTL_PATHS),
        uid: 0,
        last_persist: true,
        unit_enabled: false,
    };

    write_response(&Response {
        ok: true,
        warning: None,
        error: None,
    });

    let stdin = std::io::stdin();
    for line in stdin.lock().lines() {
        let Ok(line) = line else { break };

        let Ok(request) = serde_json::from_str::<Request>(&line) else {
            write_response(&failure("invalid request"));
            continue;
        };

        write_response(&helper.handle(request));
    }

    if !helper.last_persist && helper.table_exists() {
        let _ = helper.run_nft_args(&["delete", "table", "inet", "dropship"]);
    }

    if !helper.last_persist {
        helper.remove_cgroup();
        let _ = netscope::close();
    }

    0
}

struct Helper {
    nft: PathBuf,
    systemctl: Option<PathBuf>,
    uid: u32,
    last_persist: bool,
    unit_enabled: bool,
}

impl Helper {
    fn handle(&mut self, request: Request) -> Response {
        match request {
            Request::Hello { dynamic, uid } => self.hello(dynamic, uid),
            Request::Apply { blocks, persist } => self.apply(&blocks, persist),
            Request::EnsureScope { uid } => self.ensure_scope(uid),
        }
    }

    fn hello(&mut self, dynamic: bool, uid: u32) -> Response {
        self.uid = target_uid(uid);
        self.last_persist = !dynamic;

        let mut warning = None;

        if dynamic {
            if let Err(e) = remove_state_file() {
                warning = Some(format!("the firewall state file could not be removed: {e}"));
            }

            if let Some(systemctl) = self.systemctl.clone() {
                if unit_is_enabled(&systemctl) {
                    if let Err(e) = run_systemctl(&systemctl, &["disable", UNIT_NAME]) {
                        warning = Some(e);
                    } else {
                        self.unit_enabled = false;
                    }
                }
            }
        }

        Response {
            ok: true,
            warning,
            error: None,
        }
    }

    fn apply(&mut self, blocks: &[String], persist: bool) -> Response {
        // netscope owns the game's cgroup and one shared table with a policy
        // per consumer; two tools moving the same process disable each other
        if netscope::available() {
            return self.apply_netscope(blocks, persist);
        }

        let cgroup = cgroup_rel_path(self.uid);
        let level = cgroup_level(&cgroup);

        let ruleset = match ruleset::generate(blocks, &cgroup, level) {
            Ok(ruleset) => ruleset,
            Err(e) => return failure(e),
        };

        let Some(ruleset) = ruleset else {
            return self.clear();
        };

        // nft resolves the cgroup path when the rule loads, so the directory
        // has to exist before the block goes in
        if let Err(e) = self.ensure_cgroup() {
            return failure(e);
        }

        if let Err(e) = self.run_batch(&ruleset.base) {
            return failure(e);
        }

        let replace = format!("flush chain inet dropship output\n{}", ruleset.rules);
        if let Err(e) = self.run_batch(&replace) {
            return failure(e);
        }

        self.last_persist = persist;

        let warning = if persist {
            self.persist(&ruleset).err()
        } else {
            self.clear_persistence().err()
        };

        Response {
            ok: true,
            warning,
            error: None,
        }
    }

    fn apply_netscope(&mut self, blocks: &[String], persist: bool) -> Response {
        if blocks.is_empty() {
            return self.clear();
        }

        if let Err(e) = netscope::apply(self.uid, blocks) {
            return failure(e);
        }

        // never both: a leftover inet dropship rule would match a cgroup the
        // game no longer joins and quietly stop blocking
        let mut warning = self.clear_persistence().err();

        if self.table_exists() {
            self.remove_cgroup();
            warning = warning.or_else(|| {
                self.run_nft_args(&["delete", "table", "inet", "dropship"])
                    .err()
            });
        }

        self.last_persist = persist;

        Response {
            ok: true,
            warning,
            error: None,
        }
    }

    fn clear(&mut self) -> Response {
        let netscope = netscope::close().err();

        if self.table_exists() {
            if let Err(e) = self.run_nft_args(&["delete", "table", "inet", "dropship"]) {
                return failure(e);
            }
        }

        self.remove_cgroup();

        self.last_persist = false;

        Response {
            ok: true,
            warning: netscope.or_else(|| self.clear_persistence().err()),
            error: None,
        }
    }

    fn ensure_scope(&mut self, uid: u32) -> Response {
        self.uid = target_uid(uid);

        // netscope decides the game's cgroup; the wrapper must not move it
        if netscope::available() {
            return Response {
                ok: true,
                warning: None,
                error: None,
            };
        }

        match self.ensure_cgroup() {
            Ok(()) => Response {
                ok: true,
                warning: None,
                error: None,
            },
            Err(e) => failure(e),
        }
    }

    fn ensure_cgroup(&self) -> Result<(), String> {
        let dir = cgroup_dir(self.uid);
        if !dir.exists() {
            std::fs::create_dir(&dir)
                .map_err(|e| format!("failed to create {}: {e}", dir.display()))?;
        }

        let procs = dir.join("cgroup.procs");
        let Ok(path) = std::ffi::CString::new(procs.to_string_lossy().as_bytes()) else {
            return Err("the cgroup path contains a null byte".to_string());
        };

        // the desktop user joins the game here; root owns the directory so the
        // user cannot move the cgroup or add one underneath it
        unsafe {
            if libc::chown(path.as_ptr(), self.uid, 0) != 0 {
                return Err(format!(
                    "failed to hand {} to uid {}: {}",
                    procs.display(),
                    self.uid,
                    std::io::Error::last_os_error()
                ));
            }

            if libc::chmod(path.as_ptr(), 0o620) != 0 {
                return Err(format!(
                    "failed to make {} writable by the user: {}",
                    procs.display(),
                    std::io::Error::last_os_error()
                ));
            }
        }

        Ok(())
    }

    fn remove_cgroup(&self) {
        // a running game keeps the directory busy, which is fine: the table is
        // already gone, so nothing points at it
        let _ = std::fs::remove_dir(cgroup_dir(self.uid));
    }

    fn persist(&mut self, ruleset: &Ruleset) -> Result<(), String> {
        let Some(systemctl) = self.systemctl.clone() else {
            return Err(
                "blocks will not survive a reboot because systemd was not available".to_string(),
            );
        };

        let content = format!("{}{}", ruleset.base, ruleset.rules);
        write_state_file(&content)
            .map_err(|e| format!("blocks are active but will not survive a reboot: {e}"))?;

        let unit = UNIT.replace("%UID%", &self.uid.to_string());
        let path = unit_path();
        let changed = match std::fs::read_to_string(&path) {
            Ok(existing) => existing != unit,
            Err(_) => true,
        };

        if changed {
            std::fs::write(&path, unit)
                .map_err(|e| format!("blocks are active but will not survive a reboot: {e}"))?;
            run_systemctl(&systemctl, &["daemon-reload"])
                .map_err(|e| format!("blocks are active but will not survive a reboot: {e}"))?;
        }

        if !self.unit_enabled {
            run_systemctl(&systemctl, &["enable", UNIT_NAME])
                .map_err(|e| format!("blocks are active but will not survive a reboot: {e}"))?;
            self.unit_enabled = true;
        }

        Ok(())
    }

    fn clear_persistence(&mut self) -> Result<(), String> {
        remove_state_file()
            .map_err(|e| format!("the firewall state file could not be removed: {e}"))?;

        let Some(systemctl) = self.systemctl.clone() else {
            return Ok(());
        };

        if self.unit_enabled || unit_is_enabled(&systemctl) {
            run_systemctl(&systemctl, &["disable", "--now", UNIT_NAME])
                .map_err(|e| format!("the firewall service could not be disabled: {e}"))?;
            self.unit_enabled = false;
        }

        Ok(())
    }

    fn table_exists(&self) -> bool {
        Command::new(&self.nft)
            .args(["list", "table", "inet", "dropship"])
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .status()
            .map(|status| status.success())
            .unwrap_or(false)
    }

    fn run_batch(&self, batch: &str) -> Result<(), String> {
        let mut child = Command::new(&self.nft)
            .args(["-f", "-"])
            .stdin(Stdio::piped())
            .stdout(Stdio::null())
            .stderr(Stdio::piped())
            .spawn()
            .map_err(|e| format!("failed to start nft: {e}"))?;

        let mut write_error = None;
        if let Some(mut stdin) = child.stdin.take() {
            if let Err(e) = stdin.write_all(batch.as_bytes()) {
                write_error = Some(format!("failed to send the rules to nft: {e}"));
            }
        }

        let output = child
            .wait_with_output()
            .map_err(|e| format!("failed to run nft: {e}"))?;

        if let Some(e) = write_error {
            return Err(e);
        }

        nft_result(output)
    }

    fn run_nft_args(&self, args: &[&str]) -> Result<(), String> {
        let output = Command::new(&self.nft)
            .args(args)
            .stdout(Stdio::null())
            .stderr(Stdio::piped())
            .output()
            .map_err(|e| format!("failed to run nft: {e}"))?;

        nft_result(output)
    }
}

fn nft_result(output: Output) -> Result<(), String> {
    if output.status.success() {
        return Ok(());
    }

    let stderr = String::from_utf8_lossy(&output.stderr).trim().to_string();
    Err(if stderr.is_empty() {
        format!("nft exited with {}", output.status)
    } else {
        stderr
    })
}

/// Resolves a privileged tool. The env override exists so the behavior test
/// can point at a recorder instead of touching the host's systemd.
fn resolve(env: &str, paths: &[&str]) -> Option<PathBuf> {
    if let Some(path) = std::env::var_os(env) {
        let path = PathBuf::from(path);
        return path.is_file().then_some(path);
    }

    resolve_default(paths)
}

fn resolve_default(paths: &[&str]) -> Option<PathBuf> {
    paths
        .iter()
        .map(Path::new)
        .find(|path| path.is_file())
        .map(Path::to_path_buf)
}

fn state_dir() -> PathBuf {
    match std::env::var_os("DROPSHIP_STATE_DIR") {
        Some(dir) => PathBuf::from(dir),
        None => PathBuf::from(STATE_DIR),
    }
}

fn state_path() -> PathBuf {
    match std::env::var_os("DROPSHIP_STATE_DIR") {
        Some(dir) => PathBuf::from(dir).join("blocked.nft"),
        None => PathBuf::from(STATE_PATH),
    }
}

fn unit_path() -> PathBuf {
    match std::env::var_os("DROPSHIP_UNIT_PATH") {
        Some(path) => PathBuf::from(path),
        None => PathBuf::from(UNIT_PATH),
    }
}

fn target_uid(fallback: u32) -> u32 {
    for key in ["PKEXEC_UID", "SUDO_UID"] {
        if let Ok(value) = std::env::var(key) {
            if let Ok(uid) = value.trim().parse::<u32>() {
                return uid;
            }
        }
    }

    fallback
}

fn run_systemctl(systemctl: &Path, args: &[&str]) -> Result<(), String> {
    let output = Command::new(systemctl)
        .args(args)
        .stdout(Stdio::null())
        .stderr(Stdio::piped())
        .output()
        .map_err(|e| format!("failed to run systemctl: {e}"))?;

    if output.status.success() {
        return Ok(());
    }

    let stderr = String::from_utf8_lossy(&output.stderr).trim().to_string();
    Err(if stderr.is_empty() {
        format!("systemctl exited with {}", output.status)
    } else {
        format!("systemctl failed: {stderr}")
    })
}

fn unit_is_enabled(systemctl: &Path) -> bool {
    Command::new(systemctl)
        .args(["is-enabled", UNIT_NAME])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status()
        .map(|status| status.success())
        .unwrap_or(false)
}

fn write_state_file(content: &str) -> std::io::Result<()> {
    use std::os::unix::fs::PermissionsExt;

    let dir = state_dir();
    let path = state_path();

    std::fs::create_dir_all(&dir)?;
    std::fs::set_permissions(&dir, std::fs::Permissions::from_mode(0o755))?;
    std::fs::write(&path, content)?;
    std::fs::set_permissions(&path, std::fs::Permissions::from_mode(0o644))
}

fn remove_state_file() -> std::io::Result<()> {
    match std::fs::remove_file(state_path()) {
        Ok(()) => Ok(()),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(()),
        Err(e) => Err(e),
    }
}

fn failure(message: impl Into<String>) -> Response {
    Response {
        ok: false,
        warning: None,
        error: Some(message.into()),
    }
}

fn write_response(response: &Response) {
    let mut stdout = std::io::stdout().lock();
    let _ = serde_json::to_writer(&mut stdout, response);
    let _ = stdout.write_all(b"\n");
    let _ = stdout.flush();
}

#[cfg(test)]
mod tests {
    use super::{STATE_PATH, UNIT, cgroup_dir, cgroup_level, cgroup_rel_path};

    #[test]
    fn unit_file_uses_the_state_path() {
        let condition = format!("ConditionPathExists={STATE_PATH}");
        let command = format!("-f {STATE_PATH}");

        assert!(UNIT.contains(condition.as_str()));
        assert!(UNIT.contains(command.as_str()));
    }

    #[test]
    fn cgroup_path_and_level_agree() {
        let path = cgroup_rel_path(1000);

        assert_eq!(
            path,
            "user.slice/user-1000.slice/user@1000.service/dropship-game"
        );
        assert_eq!(cgroup_level(&path), 4);
        assert_eq!(
            cgroup_dir(1000),
            std::path::PathBuf::from(format!("/sys/fs/cgroup/{path}"))
        );
    }

    #[test]
    fn unit_file_creates_the_same_cgroup() {
        assert!(UNIT.contains("user.slice/user-%UID%.slice/user@%UID%.service/dropship-game"));
    }
}
