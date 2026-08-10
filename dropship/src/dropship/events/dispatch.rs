use crate::app::TemplateApp;
use crate::{overwatch, ping, update};

use crate::dropship::{Command, Event};
use eframe::egui;

/// this processes any pending events and their consequences
pub fn process_events(
    ctx: Option<egui::Context>,
    //
    app: &mut TemplateApp,
) {
    task(ctx, app)
}

fn task(
    ctx: Option<egui::Context>,
    //
    app: &mut TemplateApp,
) {
    if let Some(ctx) = &ctx {
        if matches!(app.installing_status, update::UpdatingStatus::Downloading) {
            // animate progress bars :c
            ctx.request_repaint();
        }
    }

    while let Ok(message) = app.logs_rx.try_recv() {
        app.logs.push(message);

        // REVIEW clear stat
        if let Some(ctx) = &ctx {
            ctx.request_repaint_after_secs(9.);
        }
    }

    while let Ok(event) = app.events_rx.try_recv() {
        // #[cfg(debug_assertions)]
        // log::debug!("event received: {:#?}", event);

        if !matches!(
            event,
            Event::Pong { .. }
                | Event::DropshipLoadingStateChange(..)
                | Event::FoundApplicationPaths(..)
                | Event::FirewallConfigApplied { .. }
        ) {
            log::debug!("[ event ] {}", event);
        }

        match event {
            //
            // an update was available
            Event::UpdateAvailable(update) => {
                app.update_available = Some(update);
            }

            // we got the ip list back and there was no error
            Event::ApiResponse(data) => {
                //
                // query ping for new ips
                {
                    data.servers.overwatch.iter().for_each(|s| {
                        if !&app.pings.contains_key(&s.ping) {
                            {
                                let ip = s.ping.clone();
                                let _ = app.commands_tx.send(Command::Ping { ip });
                            }
                        }
                    });

                    if let Some(ctx) = &ctx {
                        ctx.request_repaint_after_secs(ping::PING_TIMEOUT.as_secs_f32());
                    }
                }

                // update cache with new server info
                {
                    let cache = app.cache.get_or_insert_default();
                    cache.cached_api_data = Some(data);
                }

                // FIXME technically this should not happen in an event that is in the draw pass
                app.apply_blocked_servers_to_firewall();
            }

            // we got a ping for an ip
            Event::Pong { ip, pong } => {
                app.pings.insert(ip, pong);
            }

            // app is updating and the installation status changed
            Event::ApplicationUpdateStatusChange(s) => app.installing_status = s,

            // we now know a game/process is open/closed
            Event::ProcessOpenStatusChange {
                process_name,
                open,
                path,
            } => {
                // if we don't know about this path yet
                if let Some(path) = path {
                    if app
                        .config
                        .known_paths
                        .as_ref()
                        // case sensitive
                        // .is_none_or(|set| !set.contains(&path))
                        // case insensitive
                        .is_none_or(|known_paths| {
                            !known_paths.iter().any(|x| {
                                x.to_string_lossy().to_lowercase()
                                    == path.to_string_lossy().to_lowercase()
                            })
                        })
                    {
                        if !app.denied_paths.contains(&path) {
                            log::info!("suggesting {}", &path.display());
                            app.suggesting_path = Some(path);
                        }
                    }
                }

                match process_name.as_str() {
                    overwatch::PROCESS_NAME => {
                        app.game_open = open;

                        if !open && app.pending_firewall_sync_when_game_is_closed {
                            app.pending_firewall_sync_when_game_is_closed = false;

                            // this will dispatch a ApplyFirewallConfig command
                            app.apply_blocked_servers_to_firewall();
                        }
                    }
                    // NOTE if i add other games here i also need to change the process loop
                    // maybe event should send the previous_state instead of tracking arc<atomics>?
                    _ => {
                        log::error!(
                            "handling process change for \"{}\" is not implemented",
                            &process_name
                        );
                    }
                }
            }

            // we detected some games paths that should be synced
            Event::FoundApplicationPaths(paths) => {
                // TODO
                // REVIEW
                // delete rules that don't match these paths

                if !paths.is_empty() {
                    let known_paths = app.config.known_paths.get_or_insert_default();

                    for p in &paths {
                        known_paths.insert(p.to_owned());
                    }
                } else {
                    log::warn!("no application paths defined. please add a game executable");
                }
            }

            // the pc's configuration was updated to block these servers
            Event::FirewallConfigApplied { blocked_servers } => {
                app.config.blocked_servers = blocked_servers;

                // the first time we apply rules, do cleanup on non-dropship rules
                // we do this after since we possibly grab some paths from those
                if !app.legacy_cleanup_done {
                    tokio::task::spawn_blocking(move || {
                        match crate::firewall::legacy::delete_legacy_rules() {
                            Ok(_) => (),
                            Err(e) => {
                                log::error!("failed to delete legacy rules. ({})", e.to_string());
                            }
                        }

                        // determine firewall on/off state. if ambiguous then set to off
                        match crate::firewall::get_firewall_state_but_if_different_then_disable_them_all()
                        {
                            Ok(_) => {}
                            Err(e) => {
                                log::error!("{}", e)
                            }
                        }
                    });

                    app.legacy_cleanup_done = true;
                }
            }

            // sync status changed
            Event::DropshipLoadingStateChange(loading) => {
                app.loading = loading;
            }

            // an executable was picked
            Event::AddedExecutable(path) => {
                // add path to known paths
                app.config.known_paths.get_or_insert_default().insert(path);

                //
                app.apply_blocked_servers_to_firewall();
            } // _ => {
              //     // #[cfg(debug_assertions)]
              //     // log::error!("event not implemented {:#?}", event)

              //     log::error!("event not implemented {}", event.as_ref())
              // }
        }
    }
}
