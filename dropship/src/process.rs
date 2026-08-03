pub async fn is_process_running(process_name: &String) -> Result<bool, String> {
    let process_name = process_name.clone();

    // TODO keep one system cached in process loop
    tokio::task::spawn_blocking(move || {
        let pk = sysinfo::ProcessRefreshKind::nothing();
        let k = sysinfo::RefreshKind::nothing().with_processes(pk);
        let mut sys = sysinfo::System::new_with_specifics(k);

        // TODO store PID and only update that pid
        // FIXME
        sys.refresh_processes_specifics(sysinfo::ProcessesToUpdate::All, true, pk);
        // sys.refresh_specifics(k);

        let res = sys
            .processes_by_exact_name(std::ffi::OsStr::new(&process_name))
            .next()
            .is_some();

        Ok(res)
    })
    .await
    .map_err(|e| e.to_string())?
}
