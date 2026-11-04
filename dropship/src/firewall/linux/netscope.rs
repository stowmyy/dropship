//! Delegating the block to netscope's shared scope.
//!
//! netscope owns one cgroup per game and one nftables table with a policy per
//! consumer, so dropship can filter while ExitLink routes. Two tools that each
//! move the game into their own cgroup would silently disable each other:
//! linux gives a process exactly one cgroup, and the other tool's selector
//! then matches nothing.

use std::io::Write;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};

use super::helper::GAME_PROCESS;

pub(crate) const CONSUMER: &str = "dropship";

const NAME: &str = "dropship-overwatch";
const BINARIES: [&str; 3] = [
    "/usr/bin/netscope",
    "/usr/local/bin/netscope",
    "/bin/netscope",
];
const SOCKET: &str = "/run/netscope/control.sock";

/// Read by the unprivileged launch wrapper: the daemon socket is root-only, so
/// the helper leaves this behind when the block lives in netscope.
pub(crate) const MARKER: &str = "/run/dropship-firewall.netscope";

pub(crate) fn binary() -> Option<PathBuf> {
    if let Some(path) = std::env::var_os("DROPSHIP_NETSCOPE_BIN") {
        let path = PathBuf::from(path);
        return path.is_file().then_some(path);
    }

    BINARIES
        .iter()
        .map(Path::new)
        .find(|path| path.is_file())
        .map(Path::to_path_buf)
}
/// The helper runs as root, where the daemon socket is reachable.
pub(crate) fn available() -> bool {
    binary().is_some() && socket().exists()
}

fn socket() -> PathBuf {
    std::env::var_os("DROPSHIP_NETSCOPE_SOCKET")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from(SOCKET))
}

/// What the launch wrapper sees without root.
pub(crate) fn active() -> bool {
    Path::new(MARKER).exists()
}

pub(crate) fn open_args(uid: u32, blocks: &[String]) -> Vec<String> {
    let mut args = vec![
        "open".to_string(),
        "--consumer".to_string(),
        CONSUMER.to_string(),
        "--name".to_string(),
        NAME.to_string(),
        "--process".to_string(),
        GAME_PROCESS.to_string(),
        "--uid".to_string(),
        uid.to_string(),
        "--filter".to_string(),
        "allow".to_string(),
    ];

    // the app hands over one comma-joined string per blocked server
    for block in blocks.iter().flat_map(|block| block.split(',')) {
        let block = block.trim();
        if block.is_empty() {
            continue;
        }

        args.push("--deny-cidr".to_string());
        args.push(block.to_string());
    }

    args
}

/// Join or create the game's scope and replace this consumer's policy. `open`
/// is idempotent per consumer, so this is also the live-update path.
pub(crate) fn apply(uid: u32, blocks: &[String]) -> Result<String, String> {
    let binary = binary().ok_or("netscope is not installed")?;

    let output = Command::new(binary)
        .args(open_args(uid, blocks))
        .stdin(Stdio::null())
        .output()
        .map_err(|e| format!("netscope open: {e}"))?;

    if !output.status.success() {
        return Err(format!(
            "netscope open: {}",
            String::from_utf8_lossy(&output.stderr).trim()
        ));
    }

    let stdout = String::from_utf8_lossy(&output.stdout);
    let profile = stdout
        .split_whitespace()
        .skip_while(|word| *word != "opened")
        .nth(1)
        .unwrap_or(NAME)
        .to_string();

    mark(&profile).map_err(|e| format!("the block is active but {MARKER} was not written: {e}"))?;

    Ok(profile)
}

/// Release this consumer's claim. A missing profile is already released.
pub(crate) fn close() -> Result<(), String> {
    let profile = recorded();

    let Some(binary) = binary() else {
        unmark();
        return Ok(());
    };

    let Some(profile) = profile else {
        unmark();
        return Ok(());
    };

    let output = Command::new(binary)
        .args(["close", &profile, "--consumer", CONSUMER])
        .stdin(Stdio::null())
        .output()
        .map_err(|e| format!("netscope close: {e}"))?;

    unmark();

    if output.status.success() {
        return Ok(());
    }

    let stderr = String::from_utf8_lossy(&output.stderr);
    if stderr.contains("not found") {
        return Ok(());
    }

    Err(format!("netscope close: {}", stderr.trim()))
}

fn recorded() -> Option<String> {
    std::fs::read_to_string(MARKER)
        .ok()
        .map(|text| text.trim().to_string())
        .filter(|text| !text.is_empty())
}

fn mark(profile: &str) -> std::io::Result<()> {
    let mut file = std::fs::File::create(MARKER)?;
    writeln!(file, "{profile}")?;
    use std::os::unix::fs::PermissionsExt;
    std::fs::set_permissions(MARKER, std::fs::Permissions::from_mode(0o644))
}

fn unmark() {
    let _ = std::fs::remove_file(MARKER);
}

#[cfg(test)]
mod tests {
    use super::open_args;

    #[test]
    fn open_joins_the_scope_with_a_deny_list() {
        let args = open_args(
            1000,
            &["64.224.26.0/23".to_string(), "5.42.0.0/16".to_string()],
        );

        assert_eq!(
            args,
            [
                "open",
                "--consumer",
                "dropship",
                "--name",
                "dropship-overwatch",
                "--process",
                "Overwatch.exe",
                "--uid",
                "1000",
                "--filter",
                "allow",
                "--deny-cidr",
                "64.224.26.0/23",
                "--deny-cidr",
                "5.42.0.0/16",
            ]
        );
    }

    #[test]
    fn an_empty_deny_list_sends_no_cidrs() {
        let args = open_args(0, &[]);

        assert!(!args.iter().any(|arg| arg == "--deny-cidr"));
        assert_eq!(args.last().map(String::as_str), Some("allow"));
    }

    #[test]
    fn grouped_cidrs_are_split() {
        let args = open_args(1000, &["5.42.160.0/22,5.42.164.0/22".to_string()]);

        let cidrs: Vec<&str> = args
            .iter()
            .enumerate()
            .filter(|(index, arg)| *index > 0 && args[index - 1] == "--deny-cidr")
            .map(|(_, arg)| arg.as_str())
            .collect();

        assert_eq!(cidrs, ["5.42.160.0/22", "5.42.164.0/22"]);
    }
}
