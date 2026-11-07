use std::path::PathBuf;

pub async fn is_process_running(process_name: &String) -> Result<(bool, Option<PathBuf>), String> {
    let process_name = process_name.clone();

    // TODO keep one system cached in process loop
    tokio::task::spawn_blocking(move || {
        #[cfg(target_os = "windows")]
        {
            // let pk = sysinfo::ProcessRefreshKind::nothing();
            let pk =
                sysinfo::ProcessRefreshKind::nothing().with_exe(sysinfo::UpdateKind::OnlyIfNotSet);
            let k = sysinfo::RefreshKind::nothing().with_processes(pk);
            let mut sys = sysinfo::System::new_with_specifics(k);

            // TODO store PID and only update that pid
            // FIXME
            sys.refresh_processes_specifics(sysinfo::ProcessesToUpdate::All, true, pk);
            // sys.refresh_specifics(k);

            let res = sys
                .processes_by_exact_name(std::ffi::OsStr::new(&process_name))
                .next();

            // dbg!(res);

            let path = res
                .and_then(|first_match| first_match.exe())
                .map(|path| path.to_path_buf());

            Ok((res.is_some(), path))
        }

        #[cfg(target_os = "linux")]
        {
            // wine exposes the windows binary name only in the command line
            let pk =
                sysinfo::ProcessRefreshKind::nothing().with_cmd(sysinfo::UpdateKind::OnlyIfNotSet);
            let k = sysinfo::RefreshKind::nothing().with_processes(pk);
            let mut sys = sysinfo::System::new_with_specifics(k);

            sys.refresh_processes_specifics(sysinfo::ProcessesToUpdate::All, true, pk);

            let res = sys
                .processes()
                .values()
                .any(|p| process_matches(&process_name, p.name(), p.cmd()));

            Ok((res, None))
        }
    })
    .await
    .map_err(|e| e.to_string())?
}

#[cfg_attr(target_os = "windows", allow(dead_code))]
fn process_matches(process_name: &str, name: &std::ffi::OsStr, cmd: &[std::ffi::OsString]) -> bool {
    if name.to_string_lossy().eq_ignore_ascii_case(process_name) {
        return true;
    }

    cmd.iter().any(|arg| {
        let arg = arg.to_string_lossy();
        let basename = arg
            .rsplit(|c| c == '/' || c == '\\')
            .next()
            .unwrap_or(arg.as_ref());
        basename.eq_ignore_ascii_case(process_name)
    })
}

#[cfg(test)]
mod tests {
    use super::process_matches;
    use std::ffi::{OsStr, OsString};

    fn cmd(args: &[&str]) -> Vec<OsString> {
        args.iter().map(OsString::from).collect()
    }

    #[test]
    fn matches_by_process_name() {
        assert!(process_matches(
            "Overwatch.exe",
            OsStr::new("Overwatch.exe"),
            &[]
        ));
    }

    #[test]
    fn matches_unix_command_line_path() {
        assert!(process_matches(
            "Overwatch.exe",
            OsStr::new("wine64-preloader"),
            &cmd(&[
                "/usr/bin/wine",
                "/home/user/Games/overwatch/drive_c/Program Files (x86)/Overwatch/Overwatch.exe",
            ]),
        ));
    }

    #[test]
    fn matches_windows_command_line_path() {
        assert!(process_matches(
            "Overwatch.exe",
            OsStr::new("wine64-preloader"),
            &cmd(&["wine", "Z:\\Games\\Overwatch\\Overwatch.exe"]),
        ));
    }

    #[test]
    fn matches_case_insensitively() {
        assert!(process_matches(
            "Overwatch.exe",
            OsStr::new("OVERWATCH.EXE"),
            &[]
        ));
        assert!(process_matches(
            "Overwatch.exe",
            OsStr::new("wine64-preloader"),
            &cmd(&["wine", "Z:\\Games\\overwatch\\overwatch.EXE"]),
        ));
    }

    #[test]
    fn does_not_match_other_processes() {
        assert!(!process_matches(
            "Overwatch.exe",
            OsStr::new("wine64-preloader"),
            &cmd(&["wine", "/home/user/Games/diablo/D2R.exe"]),
        ));
    }
}
