use std::os::unix::process::CommandExt;
use std::path::PathBuf;

use super::helper::cgroup_dir;

fn cgroup_procs() -> PathBuf {
    cgroup_dir(unsafe { libc::getuid() }).join("cgroup.procs")
}

fn join(pid: u32) -> Result<(), String> {
    let path = cgroup_procs();

    std::fs::write(&path, format!("{pid}\n"))
        .map_err(|e| format!("failed to join {}: {e}", path.display()))
}

/// Puts the game in the cgroup the firewall rules match, then replaces this
/// process with it so the game keeps the pid and this wrapper does not linger.
pub(crate) async fn run(argv: Vec<String>) -> i32 {
    if argv.is_empty() {
        eprintln!("usage: dropship launch -- <command...>");
        return 2;
    }

    // netscope owns the game's cgroup when its shared scope is in use; joining
    // ours would move the game out of the scope and disable both tools
    if !super::netscope::active() {
        if join(std::process::id()).is_err() {
            // the cgroup is created by the root helper; one pkexec prompt on first use
            match super::Session::connect(false).await {
                Ok(mut session) => {
                    if let Err(e) = session.ensure_scope().await {
                        eprintln!("{e}");
                        return 1;
                    }
                }
                Err(e) => {
                    eprintln!("{e}");
                    return 1;
                }
            }

            if let Err(e) = join(std::process::id()) {
                eprintln!("{e}");
                return 1;
            }
        }
    }

    let error = std::process::Command::new(&argv[0]).args(&argv[1..]).exec();

    eprintln!("failed to start {}: {error}", argv[0]);
    1
}

#[cfg(test)]
mod tests {
    #[test]
    fn cgroup_procs_is_the_game_cgroup() {
        let uid = unsafe { libc::getuid() };
        assert_eq!(
            super::cgroup_procs(),
            std::path::PathBuf::from(format!(
                "/sys/fs/cgroup/user.slice/user-{uid}.slice/user@{uid}.service/dropship-game/cgroup.procs"
            ))
        );
    }
}
