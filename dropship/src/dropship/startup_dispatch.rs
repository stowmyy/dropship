use tokio::sync::mpsc::UnboundedSender;

use crate::{
    app::ApiCache,
    dropship::{INTERVAL_API_CHECK, INTERVAL_PROCESS_CHECK, INTERVAL_VERSION_CHECK},
    overwatch, update,
};

use super::Command;

pub fn startup_dispatch(commands_tx: &UnboundedSender<Command>, cache: &Option<ApiCache>) {
    // periodic version check
    {
        let commands_tx = commands_tx.clone();
        tokio::spawn(async move {
            let mut interval = tokio::time::interval(INTERVAL_VERSION_CHECK);
            loop {
                interval.tick().await;
                if commands_tx.send(Command::VersionCheck).is_err() {
                    break;
                }
            }
        });
    }

    // periodic game process check
    {
        let commands_tx = commands_tx.clone();
        tokio::spawn(async move {
            let mut interval = tokio::time::interval(INTERVAL_PROCESS_CHECK);
            loop {
                interval.tick().await;
                let _ = commands_tx.send(Command::ProcessCheck {
                    process_name: overwatch::PROCESS_NAME.to_string(),
                });
            }
        });
    }

    // query ips for ping
    {
        let commands_tx = commands_tx.clone();
        if let Some(cache) = &cache {
            if let Some(cache) = &cache.cached_api_data {
                cache.servers.overwatch.iter().for_each(|s| {
                    let ip = s.ping.clone();
                    let _ = commands_tx.send(Command::Ping { ip });
                });
            }
        }
    }

    // periodic api/ips fetch
    {
        let commands_tx = commands_tx.clone();
        tokio::spawn(async move {
            let mut interval = tokio::time::interval(INTERVAL_API_CHECK);
            loop {
                interval.tick().await;
                if commands_tx.send(Command::UpdateConfigFromRemote).is_err() {
                    break;
                }
            }
        });
    }

    // delete trash
    {
        tokio::spawn(async move {
            if let Ok(graveyard_binary_path) = update::graveyard_binary_path() {
                tokio::time::sleep(std::time::Duration::from_millis(900)).await;

                match tokio::fs::remove_file(&graveyard_binary_path).await {
                    Err(e) if e.kind() == std::io::ErrorKind::NotFound => (),
                    Ok(_) => log::info!("deleted previous installation executable"),
                    Err(e) => log::error!(
                        "failed to deleted previous installation executable. ({})",
                        e
                    ),
                }
            }

            if let Ok(downloading_binary_path) = update::downloading_binary_path() {
                tokio::time::sleep(std::time::Duration::from_millis(900)).await;

                match tokio::fs::remove_file(&downloading_binary_path).await {
                    Err(e) if e.kind() == std::io::ErrorKind::NotFound => (),
                    Ok(_) => log::info!("deleted previous incomplete application download"),
                    Err(e) => log::error!(
                        "failed to deleted previous incomplete application download. ({})",
                        e
                    ),
                }
            }
        });
    }
}
