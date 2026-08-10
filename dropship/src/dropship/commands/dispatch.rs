use std::sync::{Arc, atomic};

use crate::overwatch::ServerSelection;
use crate::{api, dropship, firewall, ping, process, update};

use crate::dropship::{Command, Event};
use eframe::egui;
use tokio::sync::mpsc::{UnboundedReceiver, UnboundedSender};

/// this starts a background task that consumes commands to dispatch new tasks
pub fn start_processing_commands(
    commands_rx: UnboundedReceiver<Command>,
    events_tx: UnboundedSender<Event>,
    ctx: Option<egui::Context>,
) {
    tokio::spawn(background_task(commands_rx, events_tx, ctx));
}

async fn background_task(
    mut commands_rx: UnboundedReceiver<Command>,
    events_tx: UnboundedSender<Event>,
    ctx: Option<egui::Context>,
) {
    // i could move immediate startup work here

    // NOTE i could probably send my app in a arc over here so i don't need to track state separately
    let prev_game_open = Arc::new(atomic::AtomicBool::new(false));

    while let Some(command) = commands_rx.recv().await {
        // #[cfg(debug_assertions)]
        // log::debug!("command received: {:#?}", command);

        if !matches!(
            command,
            Command::Ping { .. }
                | Command::ProcessCheck { .. }
                | Command::ApplyFirewallConfig { .. }
        ) {
            log::debug!("[command] {}", command);
        }

        let events_tx = events_tx.clone();

        // a thing happened so paint next frame just in case
        if !matches!(command, Command::ProcessCheck { .. }) {
            if let Some(ctx) = &ctx {
                ctx.request_repaint();
            }
        }

        match command {
            //
            // version update
            Command::VersionCheck => {
                tokio::spawn(async move {
                    match update::check_for_update().await {
                        Ok(info) => match info {
                            update::UpdateInfo::NoUpdate => log::info!("dropship is up to date"),
                            update::UpdateInfo::UpdateAvailable(available_update) => {
                                let _ = events_tx.send(Event::UpdateAvailable(available_update));
                            }
                        },
                        Err(e) => {
                            log::error!("failed {}. [{}]", &command, e);
                            log::error!(
                                "you may need to download an update manually from: {}",
                                dropship::GITHUB_URI
                            );
                        }
                    }
                });
            }

            // ip list comes back
            Command::UpdateConfigFromRemote => {
                tokio::spawn(async move {
                    match api::fetch_dropship_api_data().await {
                        Ok(data) => {
                            let _ = events_tx.send(Event::ApiResponse(data));
                        }
                        Err(e) => {
                            log::error!("failed {}. [{}]", &command, e);
                        }
                    }
                });
            }

            // want a ping estimate for ip
            Command::Ping { ref ip } => {
                let ip = ip.clone();
                tokio::spawn(async move {
                    let pong = ping::ping_ip(&ip).await;

                    let _ = events_tx.send(Event::Pong { ip, pong });
                });
            }

            // player wants to install a version upgrade
            Command::ApplicationUpdate {
                ref binary_download,
                ref download_total_size,
                ref downloaded_size,
            } => {
                let binary_download = binary_download.clone();
                let download_total_size = download_total_size.clone();
                let downloaded_size = downloaded_size.clone();

                tokio::spawn(async move {
                    let _ = events_tx.send(Event::ApplicationUpdateStatusChange(
                        update::UpdatingStatus::Downloading,
                    ));

                    match update::update(&binary_download, (download_total_size, downloaded_size))
                        .await
                    {
                        Ok(_) => {
                            let _ = events_tx.send(Event::ApplicationUpdateStatusChange(
                                update::UpdatingStatus::Installed,
                            ));
                        }
                        Err(e) => {
                            log::error!("failed {}. [{}]", command, &e);

                            let _ = events_tx.send(Event::ApplicationUpdateStatusChange(
                                update::UpdatingStatus::Failed(e),
                            ));
                        }
                    }
                });
            }

            // want to check if a process is open
            // this is used to detect open games
            Command::ProcessCheck { ref process_name } => {
                let process_name = process_name.clone();
                let prev_game_open = prev_game_open.clone();

                tokio::spawn(async move {
                    match process::is_process_running(&process_name).await {
                        Ok((open, path)) => {
                            let p = prev_game_open.swap(open, atomic::Ordering::Relaxed);

                            if p != open {
                                let _ = events_tx.send(Event::ProcessOpenStatusChange {
                                    process_name,
                                    open,
                                    path,
                                });
                            }
                        }
                        Err(e) => {
                            log::error!("failed {}. [{}]", &command, &e);
                        }
                    }
                });
            }

            // we want to sync server selection with pc's configuration
            Command::ApplyFirewallConfig {
                blocked_servers,
                already_known_paths,
            } => {
                // FIXME probably should have a rc so it can't conflict state and infinitely think it's loading
                // if the queue is drained out of order :<
                let _ = events_tx.send(Event::DropshipLoadingStateChange(true));

                tokio::spawn(async move {
                    match firewall::win::apply_blocked_ips(&blocked_servers, &already_known_paths) {
                        Ok(affected_paths) => {
                            let _ = events_tx.send(Event::FoundApplicationPaths(affected_paths));

                            {
                                let blocked_servers = {
                                    let mut b = ServerSelection::none();
                                    for s in &blocked_servers {
                                        b.set_bit(s.bit);
                                    }
                                    b
                                };
                                let _ = events_tx
                                    .send(Event::FirewallConfigApplied { blocked_servers });
                            }
                        }
                        Err(e) => {
                            log::error!("updating this pc's firewall failed. ({})", e.to_string());
                        }
                    }

                    let _ = events_tx.send(Event::DropshipLoadingStateChange(false));
                });
            }

            // player wants to add another exe to dropship
            // probably isn't one that was autodetected
            Command::AddExecutable { ref path } => {
                let path = path.clone();

                // FIXME middleware does not really make sense here

                let _ = events_tx.send(Event::AddedExecutable(path));
            } // _ => {
              //     // #[cfg(debug_assertions)]
              //     // log::error!("command not implemented {:#?}", command);

              //     log::error!("command not implemented {}", command.as_ref());
              // }
        }
    }
}
