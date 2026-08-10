use std::path::PathBuf;

pub async fn is_process_running(process_name: &String) -> Result<(bool, Option<PathBuf>), String> {
    let process_name = process_name.clone();

    // TODO keep one system cached in process loop
    tokio::task::spawn_blocking(move || {
        // let pk = sysinfo::ProcessRefreshKind::nothing();
        let pk = sysinfo::ProcessRefreshKind::nothing().with_exe(sysinfo::UpdateKind::OnlyIfNotSet);
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
    })
    .await
    .map_err(|e| e.to_string())?
}
