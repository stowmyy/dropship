use eframe::egui;

use crate::api;
use crate::overwatch::ServerSelection;

use crate::assets;

/// draw the stars here
pub fn server_list_indicators(
    ui: &mut egui::Ui,
    servers: &[api::KnownServer],
    desired_blocked_servers: &ServerSelection,
    actually_blocked_servers: ServerSelection,
) {
    servers.into_iter().enumerate().for_each(|(i, server)| {
        let blocked = actually_blocked_servers.has(server);
        let pending = desired_blocked_servers.has(server) != actually_blocked_servers.has(server);

        let color = if pending {
            crate::visuals::color_secondary_faded(i)
        } else {
            if blocked {
                // ui.visuals().weak_text_color()
                egui::Color32::from_black_alpha(9)
            } else {
                crate::visuals::color_active(i)
            }
        };

        let text = if pending {
            format!("{} (waiting for game to close..)", server.token)
        } else {
            if blocked {
                format!("{} (blocked)", server.token)
            } else {
                server.token.clone()
            }
        };

        ui.add(
            egui::Image::new(assets::ICON_STAR)
                .fit_to_exact_size(egui::vec2(16., 16.))
                .tint(color),
        )
        .on_hover_text_at_pointer(text);
    });
}

/// draw a writable selection
pub fn server_list(
    ui: &mut egui::Ui,
    servers: &[api::KnownServer],
    desired_blocked_servers: &mut ServerSelection,
    blocked_servers: ServerSelection,
    //
    pings: &std::collections::HashMap<String, Result<f32, String>>,
) -> bool {
    //
    let mut selection_changed = false;

    servers.iter().enumerate().for_each(|(i, server)| {
        ui.horizontal(|ui| {
            ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                if let Some(button) = server_list_item(
                    ui,
                    server,
                    desired_blocked_servers.has(server),
                    blocked_servers.has(server),
                    i,
                    pings,
                ) {
                    if button.hovered() || button.clicked() {
                        let unblocked_servers_remaining = servers
                            .iter()
                            .filter(|x| !desired_blocked_servers.has(x))
                            .count();

                        if !desired_blocked_servers.has(server) && unblocked_servers_remaining == 1
                        {
                            if button.clicked() {
                                log::warn!(
                                    "cannot block {} because all servers would be blocked",
                                    server.title.to_ascii_lowercase()
                                );
                            } else if button.secondary_clicked() {
                                desired_blocked_servers.solo(server);
                                selection_changed = true;
                            }
                        } else {
                            if button.clicked() {
                                desired_blocked_servers.toggle(server);
                                selection_changed = true;
                            } else if button.secondary_clicked() {
                                desired_blocked_servers.solo_invert(server);
                                selection_changed = true;
                            }
                        }

                        button.on_hover_text_at_pointer({
                            let text = if desired_blocked_servers.has(server)
                                != blocked_servers.has(server)
                            {
                                format!("{} (waiting for game to close..)", server.token)
                            } else {
                                if desired_blocked_servers.has(server) {
                                    format!("{} (blocked)", server.token)
                                } else {
                                    server.token.clone()
                                }
                            };

                            text
                        });
                    }
                }
            })
        });
    });

    selection_changed
}

/// draw a writable server
pub fn server_list_item(
    ui: &mut egui::Ui,
    server: &api::KnownServer,
    wants_blocked: bool,
    is_blocked: bool,
    i: usize,
    //
    pings: &std::collections::HashMap<String, Result<f32, String>>,
) -> Option<egui::Response> {
    let mut b = None;

    ui.horizontal(|ui| {
        ui.scope(|ui| {
            let blocked = is_blocked;
            let pending = wants_blocked != is_blocked;

            if pending {
                if !wants_blocked {
                    // TODO
                }
            } else {
                if blocked {
                    ui.style_mut().visuals.widgets.inactive.weak_bg_fill =
                        egui::Color32::from_black_alpha(0);
                    ui.style_mut().visuals.widgets.active.weak_bg_fill =
                        egui::Color32::from_black_alpha(40);
                    ui.style_mut().visuals.widgets.hovered.weak_bg_fill =
                        egui::Color32::from_black_alpha(20);

                    ui.style_mut().visuals.widgets.hovered.weak_bg_fill =
                        crate::visuals::color_secondary_faded(i);

                    ui.style_mut().visuals.override_text_color =
                        Some(ui.style_mut().visuals.weak_text_color());
                } else {
                    // ui.style_mut().visuals.override_text_color =
                    //     Some(crate::visuals::color_primary(i));
                    ui.style_mut().visuals.widgets.inactive.weak_bg_fill =
                        crate::visuals::color_inactive(i);
                    ui.style_mut().visuals.widgets.active.weak_bg_fill =
                        crate::visuals::color_active(i);
                    ui.style_mut().visuals.widgets.hovered.weak_bg_fill =
                        crate::visuals::color_hovered(i);
                }
            }

            ui.style_mut().interaction.selectable_labels = false;

            // let text = if pending { "pending" } else { &server.title };
            let text = &server.title;

            let button = ui.add(egui::Button::new(text).min_size(egui::vec2(
                ui.available_width(),
                ui.spacing().interact_size.y,
            )));

            let rect = button.rect;

            b = Some(button);

            let mut child =
                ui.new_child(egui::UiBuilder::new().max_rect(rect.shrink2(egui::vec2(8.0, 0.0))));

            child.horizontal(|ui| {
                // ui.label(if blocked { "<blocked />" } else { "" });

                if pending {
                    ui.add(egui::Spinner::new().size(16.));
                } else {
                    if blocked {
                        // ui.label("}: blocked :{");
                        ui.add(
                            egui::Image::new(assets::ICON_BAN)
                                .fit_to_exact_size(egui::vec2(16., 16.))
                                .tint(crate::visuals::color_primary(i)),
                        );
                    } else {
                        ui.add(
                            egui::Image::new(assets::ICON_STAR)
                                // TODO font size?
                                .fit_to_exact_size(egui::vec2(16., 16.))
                                .tint(crate::visuals::color_secondary_faded(i)),
                        );
                    }
                }

                {
                    let (icon, _tooltip, tint) = {
                        match pings.get(&server.ping) {
                            // ping ok
                            Some(Ok(ms)) => (
                                Some(crate::ping_icon::ping_icon(*ms)),
                                format!("{ms:.0}ms"),
                                None,
                            ),

                            // ping error
                            Some(Err(e)) => (
                                // Some(crate::ping_icon::ping_icon(f32::NAN)),
                                None,
                                e.clone(),
                                // Some(ui.visuals().error_fg_color),
                                None,
                            ),

                            // ping pending
                            None => (
                                Some(crate::ping_icon::ping_icon_cycle(ui.time())),
                                "pinging".to_string(),
                                Some(ui.visuals().weak_text_color()),
                            ),
                        }
                    };

                    let tag_color = tint.unwrap_or(ui.style().visuals.text_color());

                    ui.with_layout(egui::Layout::left_to_right(egui::Align::Center), |ui| {
                        let _icon = if let Some(icon) = icon {
                            Some(
                                ui.add(
                                    egui::Image::new(icon)
                                        .tint(tag_color)
                                        .fit_to_exact_size(egui::vec2(16.0, 16.0)),
                                ),
                            )
                        } else {
                            None
                        };

                        ui.add_space(4.);

                        {
                            // let label = ui.colored_label(
                            //     tag_color,
                            //     server.token.to_ascii_lowercase(), // .to_ascii_uppercase()
                            // );

                            // // ping

                            // if let Some(icon) = icon {
                            //     icon.union(label);
                            // }
                        }
                    });

                    // testing
                    // tag.response.on_hover_text_at_pointer(tooltip);
                }
            });
        });
    });

    b
}
