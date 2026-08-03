use crate::{
    api::{self, KnownServer},
    assets, components,
    dropship::{self, startup_dispatch},
    firewall::{self, applications::ApplicationType},
    logger,
    overwatch::ServerSelection,
    visuals,
};
use eframe::egui;
use std::{
    collections::{HashMap, HashSet},
    path::PathBuf,
    sync::{Arc, atomic},
};
use tokio::sync::mpsc::{self, UnboundedReceiver, UnboundedSender};

use crate::update;

/* todo

    [ ] make sure the svgs i downloaded all have 1:1 viewbox 640x640. in chromium

    [ ] log in ui, removing mina rules etc

    [ ] browse memory region for server strings? might have ip addrs nearby
    [ ] sort by ping button

    [ ] detect windows firewall disabled or external firewall software

    [ ] make sure it works offline, managing servers etc

    [ ] automatically get dacom kr? people have been having issues

    [ ] custom rules?
    [ ] fetch gpc manually button/background task?

    [ ] flush dns cache button?
        some users report unblocking but remaining at high ping to their closest server

    [ ] rotating tip messages. (like m2 to play on a server)
    [ ] "most likely to play on" message

    [ ] log all fs paths and operations

    [ ] enforce only one instance at a time (and fix updater so it works after this change)
*/

const CACHE_KEY: &str = "cache";

#[derive(serde::Deserialize, serde::Serialize)]
#[serde(default)] // if we add new fields, give them default values when deserializing old state
pub struct DropshipConfig {
    zoom: f32,
    welcomed: bool,
    always_show_welcome: bool,
    pub desired_blocked_servers: ServerSelection,
    pub(crate) blocked_servers: ServerSelection,
    pub known_paths: Option<HashSet<PathBuf>>,
    starting_tab: usize,
    theme: Option<visuals::Theme>, // none is system theme
}

impl Default for DropshipConfig {
    fn default() -> Self {
        Self {
            zoom: 1.,
            welcomed: false,
            always_show_welcome: false,
            desired_blocked_servers: ServerSelection::none(),
            blocked_servers: ServerSelection::none(),
            known_paths: None,
            starting_tab: 0,
            theme: None,
        }
    }
}

const TAB_LOG: usize = 1;

pub struct TemplateApp {
    //
    pub(crate) commands_tx: UnboundedSender<dropship::Command>,
    pub(crate) events_rx: UnboundedReceiver<dropship::Event>,
    pub(crate) logs_rx: mpsc::UnboundedReceiver<logger::Message>,
    pub(crate) config: DropshipConfig,
    pub(crate) cache: Option<ApiCache>,

    //
    download_total_size: Arc<atomic::AtomicU64>,
    downloaded_size: Arc<atomic::AtomicU64>,
    pub(crate) game_open: bool,
    pub(crate) installing_status: update::UpdatingStatus,
    // pub(crate) known_applications: Option<Vec<applications::Application>>,
    // pub(crate) known_applications: Option<HashSet<PathBuf>>,
    pub(crate) logs: Vec<logger::Message>,
    pub(crate) pings: HashMap<String, Result<f32, String>>,
    pub(crate) update_available: Option<update::AvailableUpdate>,

    //
    export_ips_modal: bool,
    modal_manage_path: Option<PathBuf>,
    hide_update: bool,
    modal_welcome_page: Option<u8>,
    notice_expanded: bool,
    restart_requested: bool,
    tab: usize,
    pub(crate) loading: bool,
    pub(crate) pending_firewall_sync_when_game_is_closed: bool,
    pub(crate) legacy_cleanup_done: bool,
    // pub(crate) cached_lowest_ping_server: Option<KnownServer>,
    prev_system_theme: Option<egui::Theme>,
    //
}

#[derive(serde::Deserialize, serde::Serialize, Default, Clone)]
pub struct ApiCache {
    pub cached_api_data: Option<api::DropshipApiData>,
}

impl TemplateApp {
    pub fn new(
        cc: &eframe::CreationContext<'_>,
        logs_rx: mpsc::UnboundedReceiver<logger::Message>,
    ) -> Self {
        cc.egui_ctx.set_fonts(crate::visuals::fonts());

        // if let Some(c) = egui::ViewportCommand::center_on_screen(&cc.egui_ctx) {
        //     cc.egui_ctx.send_viewport_cmd(c);
        // }

        // load previous app state
        let (config, cache) = {
            if let Some(storage) = cc.storage {
                let config = if let Some(config) =
                    eframe::get_value::<DropshipConfig>(storage, eframe::APP_KEY)
                {
                    config
                } else {
                    log::warn!("failed to deserialize dropship configuration");
                    // log::warn!("didn't find a valid dropship config file");
                    Default::default()
                };

                // failable cache
                let cache = if let Some(value) =
                    eframe::get_value::<Option<ApiCache>>(storage, CACHE_KEY)
                {
                    value
                } else {
                    log::warn!("failed to load cached data");
                    None
                };

                (config, cache)
            } else {
                log::warn!("couldn't find an existing dropship file");
                Default::default()
            }
        };

        {
            let size = egui::vec2(crate::APP_WIDTH, crate::APP_HEIGHT);
            cc.egui_ctx
                .send_viewport_cmd(egui::ViewportCommand::InnerSize(size));
            cc.egui_ctx.set_zoom_factor(config.zoom);
        }

        let (commands_tx, commands_rx) = mpsc::unbounded_channel::<dropship::Command>();
        let (events_tx, events_rx) = mpsc::unbounded_channel::<dropship::Event>();

        dropship::start_processing_commands(commands_rx, events_tx, Some(cc.egui_ctx.clone()));

        startup_dispatch(&commands_tx, &cache);

        let mut app = Self {
            //
            commands_tx,
            events_rx,
            logs_rx,
            cache,
            config,

            //
            download_total_size: Arc::new(atomic::AtomicU64::new(0)),
            downloaded_size: Arc::new(atomic::AtomicU64::new(0)),
            game_open: false,
            installing_status: update::UpdatingStatus::NotActive,
            logs: vec![],
            pings: HashMap::new(),
            update_available: None,

            //
            export_ips_modal: false,
            modal_manage_path: None,
            hide_update: false,
            modal_welcome_page: None,
            notice_expanded: false,
            restart_requested: false,
            tab: 0,
            loading: false,
            pending_firewall_sync_when_game_is_closed: false,
            legacy_cleanup_done: false,
            // cached_lowest_ping_server: None,
            prev_system_theme: cc.egui_ctx.system_theme(),
            //
        };

        {
            if !app.config.welcomed || app.config.always_show_welcome {
                app.modal_welcome_page = Some(0);
            }

            app.tab = app.config.starting_tab;
        }

        app.apply_theme(&cc.egui_ctx);

        app
    }

    pub fn known_servers(&self) -> &[api::KnownServer] {
        if let Some(cache) = &self.cache {
            if let Some(data) = &cache.cached_api_data {
                return data.servers.overwatch.as_slice();
            }
        }
        &[]
    }

    fn apply_zoom(&mut self, ui: &egui::Context, zoom: f32) {
        ui.set_zoom_factor(zoom);
        self.config.zoom = zoom;

        let active = ui.zoom_factor();

        let structural_adjustment = zoom / active;
        let size = egui::vec2(
            crate::APP_WIDTH * structural_adjustment,
            crate::APP_HEIGHT * structural_adjustment,
        );
        ui.send_viewport_cmd(egui::ViewportCommand::InnerSize(size));
    }

    // prev frame values, without need for ctx
    fn _get_theme(&self) -> visuals::Theme {
        let theme = {
            if let Some(t) = self.config.theme {
                t
            } else {
                match self.prev_system_theme {
                    Some(t) => match t {
                        egui::Theme::Dark => visuals::Theme::Dark,
                        egui::Theme::Light => visuals::Theme::Light,
                    },
                    None => visuals::Theme::default(),
                }
            }
        };

        theme
    }

    fn get_theme(&self, ui: &egui::Context) -> visuals::Theme {
        let theme = {
            if let Some(t) = self.config.theme {
                t
            } else {
                match ui.system_theme() {
                    Some(t) => match t {
                        egui::Theme::Dark => visuals::Theme::Dark,
                        egui::Theme::Light => visuals::Theme::Light,
                    },
                    None => visuals::Theme::default(),
                }
            }
        };

        theme
    }

    fn apply_theme(&mut self, ui: &egui::Context) {
        let theme = self.get_theme(ui);

        ui.all_styles_mut(move |style| crate::visuals::visuals(style, theme));
    }
}

pub const HERO_BG_SIZE: egui::Vec2 = egui::vec2(1920.0, 885.0);

impl eframe::App for TemplateApp {
    fn persist_egui_memory(&self) -> bool {
        false
    }

    fn save(&mut self, storage: &mut dyn eframe::Storage) {
        eframe::set_value(storage, eframe::APP_KEY, &self.config);

        // failable cache
        eframe::set_value(storage, CACHE_KEY, &self.cache);

        if self.restart_requested {
            log::info!("restart requested");

            if let Ok(installed_binary_path) = std::env::current_exe() {
                std::process::Command::new(installed_binary_path)
                    .spawn()
                    .map_err(|e| {
                        log::error!("{}", e);
                        e
                    })
                    .ok();
            }
        }
    }

    fn clear_color(&self, _visuals: &egui::Visuals) -> [f32; 4] {
        // egui::Color32::from_rgba_unmultiplied(193, 197, 209, 255).to_normalized_gamma_f32()

        match self._get_theme() {
            visuals::Theme::Light => {
                egui::Color32::from_rgba_unmultiplied(236, 239, 246, 255).to_normalized_gamma_f32()
            }
            visuals::Theme::Dark => {
                egui::Color32::from_rgba_unmultiplied(14, 18, 28, 255).to_normalized_gamma_f32()
            }
        }
    }

    // happens before every ui()
    fn logic(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        // process events
        dropship::process_events(Some(ctx.clone()), self);

        // when using system theme, check for changes
        if self.config.theme.is_none()
            && let Some(system_theme) = ctx.system_theme()
        {
            if self.prev_system_theme != Some(system_theme) {
                log::debug!("pc theme change detected");
                self.apply_theme(ctx);
            }
        }

        // cache previous frame's theme
        self.prev_system_theme = ctx.system_theme();
    }

    /// called each time the ui needs repainting, which may be many times per second.
    fn ui(&mut self, ui: &mut egui::Ui, frame: &mut eframe::Frame) {
        let esc_pressed: bool = ui.ctx().input(|i| i.key_pressed(egui::Key::Escape));

        if esc_pressed
            && !ui.any_popup_open()
            && !self.export_ips_modal
            && self.modal_welcome_page.is_none()
            && !self.should_show_update_modal()
            && self.modal_manage_path.is_none()
        {
            ui.send_viewport_cmd(egui::ViewportCommand::Close);
        }

        let theme = self.get_theme(ui);

        // let hero_bg = egui::Image::new(egui::include_image!("../assets/images/hero-bg.png")).tint();
        let hero_bg = {
            match self.get_theme(ui) {
                visuals::Theme::Light => {
                    egui::Image::new(egui::include_image!("../assets/images/hero-bg.png"))
                        .show_loading_spinner(false)
                }
                visuals::Theme::Dark => {
                    egui::Image::new(egui::include_image!("../assets/images/hero-bg_dark.png"))
                        .show_loading_spinner(false)
                }
            }
        };
        // .maintain_aspect_ratio(true)
        // .max_height(ui.viewport_rect().height())
        let hero_pos = [-490., -90.];
        hero_bg.paint_at(
            ui,
            [
                [0.0 + hero_pos[0], 0.0 + hero_pos[1]].into(),
                // [ui.viewport_rect().width(), ui.viewport_rect().height()].into(),
                HERO_BG_SIZE.to_pos2() + egui::vec2(hero_pos[0], hero_pos[1]),
            ]
            .into(),
        );

        // footer
        egui::Panel::bottom("bottom_panel")
            .frame(egui::Frame::default().outer_margin(egui::Margin::symmetric(16, 16)))
            .show(ui, |ui| {
                ui.horizontal(|ui| {
                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        egui::Frame::group(ui.style())
                            .inner_margin(0.)
                            .outer_margin(0.)
                            .show(ui, |ui| {
                                ui.scope(|ui| {
                                    ui.disable();

                                    // m2
                                    {
                                        let icon_size = 16.;

                                        let icon = egui::Image::new(assets::ICON_M2)
                                            .fit_to_exact_size(egui::vec2(icon_size, icon_size))
                                            .tint(ui.visuals().text_color());

                                        let button = egui::Button::image(icon);
                                        ui.add(button);

                                        ui.label("toggle others");
                                    }

                                    ui.separator();

                                    // m1
                                    {
                                        let icon_size = 16.;

                                        let icon = egui::Image::new(assets::ICON_M1)
                                            .fit_to_exact_size(egui::vec2(icon_size, icon_size))
                                            .tint(ui.visuals().text_color());

                                        let button = egui::Button::image(icon);
                                        ui.add(button);

                                        ui.label("toggle");
                                    }

                                    ui.separator();

                                    // exit
                                    {
                                        if ui.button("esc").clicked() {
                                            ui.send_viewport_cmd(egui::ViewportCommand::Close);
                                        };

                                        ui.label("close");
                                    }
                                })
                            });
                    });
                });

                ui.with_layout(egui::Layout::left_to_right(egui::Align::Center), |ui| {
                    ui.with_layout(egui::Layout::bottom_up(egui::Align::LEFT), |ui| {
                        ui.horizontal(|ui| {
                            ui.spacing_mut().item_spacing.x = 0.0;
                            ui.label("written by stormy.");
                        });
                        ui.hyperlink_to(
                            egui::RichText::new("> source code <").small(),
                            dropship::GITHUB_URI,
                        )
                        .on_hover_text_at_pointer(dropship::GITHUB_URI)
                    });
                });
            });

        egui::Panel::top("top_panel")
            .frame(egui::Frame::default().outer_margin(egui::Margin {
                top: 16,
                left: 16,
                right: 16,
                bottom: 16,
            }))
            .show(ui, |ui| {
                ui.horizontal(|ui| {
                    // ui.label("<version />");
                    ui.label(format!("v{}", env!("CARGO_PKG_VERSION")));

                    egui::warn_if_debug_build(ui);

                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        // ui.label("@stormyy_ow");
                        self.stat(ui);
                    });
                });
            });

        // ui.show_viewport_deferred(
        //     egui::ViewportId::from_hash_of("my_deferred_window"), // Unique ID
        //     egui::ViewportBuilder::default()
        //         .with_title("Deferred Window")
        //         .with_inner_size([400.0, 300.0])
        //         .with_drag_and_drop(false),
        //     |ui, class| {
        //         // This closure defines the UI for the new window
        //         egui::CentralPanel::default().show_inside(ui, |ui| {
        //             ui.heading("This is a performant separate window!");
        //         });
        //     },
        // );

        // side
        egui::Panel::right("side_panel")
            // .frame(egui::Frame::default())
            .frame(egui::Frame::default().outer_margin(egui::Margin::same(8)))
            .exact_size(300.)
            .resizable(false)
            // .default_size(300.)
            // .min_size(200.)
            // .max_size(400.)
            .show(ui, |ui| {


                egui::Panel::bottom("actions")
                    .frame(
                        egui::Frame::default()
                            .outer_margin(egui::Margin::ZERO)
                            .inner_margin(egui::Margin::ZERO),
                    )
                    .show(ui, |ui| {
                        ui.separator();

                        ui.scope(|ui| {
                                let button = egui::Button::new("disable dropship");
                                let button =
                                    ui.add_sized(egui::vec2(ui.available_width() - 8. - 4., 16.0), button)
                                        .on_hover_text("if you are ever failing to connect to a server, quickly pressing this will prevent a competitive ban");

                                if button.clicked() {
                                    self.force_unblock_all();
                                }
                        });
                    });

                egui::Panel::top("server_list")
                    .frame(
                        egui::Frame::default()
                            .outer_margin(egui::Margin::ZERO)
                            .inner_margin(egui::Margin::ZERO),
                    )
                    .exact_size(ui.available_height())
                    .show(ui, |ui| {
                        ui.label("i want to play on..");

                        ui.separator();

                        self.servers(ui);
                    });

            });

        // main
        egui::CentralPanel::default()
            .frame(
                egui::Frame::default()
                    .outer_margin(egui::Margin::symmetric(32, 0))
                    .inner_margin(egui::Margin::ZERO),
            )
            .show(ui, |ui| {
                ui.vertical(|ui| {

                    ui.heading({
                        match &self.config.known_paths.as_ref().map_or(0, |x| x.len()) {
                            0 => "warning: you have no games added",
                            1 => "this game",
                            _ => "these games",
                        }
                    });

                    // ui.disable();
                    self.applications(ui);
                });

                ui.heading("will only play on those servers ->");

                ui.horizontal(|ui| {
                    components::server_list_item::server_list_indicators(
                        ui,
                        &self.known_servers(),
                        &self.config.desired_blocked_servers,
                        self.config.blocked_servers,
                        //
                        theme,
                    );

                    let blocked = self
                        .known_servers()
                        .iter()
                        .filter(|x| self.config.desired_blocked_servers.has(x))
                        .count();
                    ui.label(format!("({} blocked)", blocked));
                });

                ui.separator();

                ui.label("this configuration will be applied to the selected applications above. you do not need to keep this window open.");

                // if let Some(lowest_ping_server) = &self.cached_lowest_ping_server {
                if let Some(lowest_ping_server) = self.get_most_likely_to_play_on() {
                    // ui.separator();
                    ui.label(format!("you are most likely to play on \"{}\" ({})", &lowest_ping_server.title, &lowest_ping_server.token));
                }

                ui.separator();


                egui::Panel::bottom("tabs")
                    .frame(egui::Frame::default().outer_margin(egui::Margin::same(0)))
                    .show(ui, |ui| {
                        self.tabs(ui, frame);
                    });
            });

        if let Some(page) = self.modal_welcome_page {
            self.welcome(ui, page);
        }

        self.updater(ui);
    }
}

impl TemplateApp {
    fn _apply_blocked_servers_to_firewall(
        blocked_servers: &ServerSelection,
        known_servers: &[KnownServer],
        already_known_paths: &Option<HashSet<PathBuf>>,
        commands_tx: &UnboundedSender<dropship::Command>,
    ) {
        let blocked_servers = known_servers
            .iter()
            .filter(|x| blocked_servers.has(x))
            .map(|x| x.clone())
            .collect();

        let already_known_paths = already_known_paths.clone().unwrap_or_default();

        let _ = commands_tx.send(dropship::Command::ApplyFirewallConfig {
            blocked_servers,
            already_known_paths,
        });
    }

    pub fn apply_blocked_servers_to_firewall(&mut self) {
        if self.game_open {
            self.pending_firewall_sync_when_game_is_closed = true;
        } else {
            Self::_apply_blocked_servers_to_firewall(
                &self.config.desired_blocked_servers,
                self.known_servers(),
                &self.config.known_paths,
                &self.commands_tx,
            );
        }
    }

    pub fn force_unblock_all(&mut self) {
        self.config.desired_blocked_servers = ServerSelection::none();

        Self::_apply_blocked_servers_to_firewall(
            &self.config.desired_blocked_servers,
            self.known_servers(),
            &self.config.known_paths,
            &self.commands_tx,
        );
    }

    fn stat(&mut self, ui: &mut egui::Ui) {
        let mut widget = None;

        if !self.logs.is_empty() {
            let now = chrono::Local::now();
            let window = now - chrono::Duration::seconds(9);

            if let Some(error) = self
                .logs
                .iter()
                .rev()
                .find(|m| m.level == log::Level::Error && m.time >= window)
            {
                widget = Some(
                    egui::Label::new(
                        egui::RichText::new(&error.message).color(ui.visuals().error_fg_color),
                    )
                    .truncate(),
                );
            } else if let Some(warning) = self
                .logs
                .iter()
                .rev()
                .find(|m| m.level == log::Level::Warn && m.time >= window)
            {
                widget = Some(
                    egui::Label::new(
                        egui::RichText::new(&warning.message).color(ui.visuals().warn_fg_color),
                    )
                    .truncate(),
                )
            } else if let Some(message) = self.logs.iter().rev().find(|m| m.time >= window) {
                widget = Some(
                    egui::Label::new(
                        egui::RichText::new(
                            message.message.strip_prefix("[ event ] ").unwrap_or(
                                &message
                                    .message
                                    .strip_prefix("[command] ")
                                    .unwrap_or(&message.message),
                            ),
                        )
                        .color(ui.visuals().weak_text_color()),
                    )
                    .truncate(),
                )
            }
        }

        if let Some(widget) = widget {
            if self.tab != TAB_LOG {
                if ui
                    .add(widget)
                    .on_hover_cursor(
                        ui.style()
                            .visuals
                            .interact_cursor
                            .unwrap_or(egui::CursorIcon::PointingHand),
                    )
                    .clicked()
                {
                    self.tab = TAB_LOG;
                }
            } else {
                ui.add(widget);
            }
        }

        // ui.label("=^.^=");
    }

    fn draw_path(path: &PathBuf, ui: &mut egui::Ui) -> bool {
        let mut clicked = false;

        // let p = path.to_string_lossy().to_lowercase();
        let p = path.display().to_string();
        let ty = {
            if p.contains("_retail_") {
                ApplicationType::Blizzard
            } else if p.contains("steamapps") {
                ApplicationType::Valve
            } else {
                ApplicationType::Unknown
            }
        };

        ui.horizontal(|ui| {
            ui.scope(|ui| {
                ui.spacing_mut().item_spacing.x = ui.style().spacing.item_spacing.y;
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    let image = match ty {
                        firewall::applications::ApplicationType::Blizzard => {
                            assets::COMPANY_ICON_BATTLENET
                        }
                        firewall::applications::ApplicationType::Valve => {
                            assets::COMPANY_ICON_STEAM
                        }
                        _ => assets::GAME_ICON_OVERWATCH,
                    };

                    let button = egui::Button::image_and_text(
                        egui::Image::new(image).fit_to_exact_size(egui::vec2(16.0, 16.0)),
                        &path.display().to_string(),
                        // REVIEW lower case here?
                    )
                    .wrap_mode(egui::TextWrapMode::Truncate)
                    .min_size(egui::vec2(
                        ui.available_width(),
                        ui.spacing().interact_size.y,
                    ))
                    .gap(8.);
                    clicked = ui.add(button).clicked();
                });
            });
        });

        clicked
    }

    pub fn get_most_likely_to_play_on(&self) -> Option<&KnownServer> {
        let mut lowest_ping_server = None;
        let mut lowest_ping = f32::INFINITY;

        for s in self.known_servers() {
            if self.config.blocked_servers.has(s) {
                continue;
            }

            if let Some(ping) = self.pings.get(&s.ping) {
                if let Ok(ping) = ping {
                    if *ping < lowest_ping {
                        lowest_ping = *ping;
                        lowest_ping_server = Some(s);
                    }
                }
            }
        }

        lowest_ping_server
    }

    fn applications(&mut self, ui: &mut egui::Ui) {
        egui::Frame::group(ui.style()).show(ui, |ui| {
            //

            match &self.config.known_paths {
                Some(paths) => {
                    egui::ScrollArea::vertical()
                        .content_margin(egui::Margin {
                            right: 4 + 8, // gap + width + margin
                            top: 0,
                            left: 0,
                            bottom: 0,
                        })
                        .auto_shrink([false, true])
                        .max_height(160.)
                        .show(ui, |ui| {
                            paths.into_iter().for_each(|path| {
                                if Self::draw_path(&path, ui) {
                                    self.modal_manage_path = Some(path.clone());
                                }
                            });
                        });
                }
                None => {
                    ui.spinner();
                }
            }

            let theme = self.get_theme(ui);

            // new
            ui.horizontal(|ui| {
                ui.scope(|ui| {
                    //

                    // if the list isn't empty, fade it
                    if !&self
                        .config
                        .known_paths
                        .as_ref()
                        .is_some_and(|x| x.is_empty())
                    {
                        ui.style_mut().visuals.widgets.inactive.weak_bg_fill =
                            visuals::from_theme_alpha(theme, 0);
                        ui.style_mut().visuals.widgets.active.weak_bg_fill =
                            visuals::from_theme_alpha(theme, 40);
                        ui.style_mut().visuals.widgets.hovered.weak_bg_fill =
                            visuals::from_theme_alpha(theme, 20);

                        ui.style_mut().visuals.override_text_color =
                            Some(ui.style_mut().visuals.weak_text_color());
                    }

                    ui.spacing_mut().item_spacing.x = ui.style().spacing.item_spacing.y;
                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                        // {
                        //     let icon_size = 16.;

                        //     let icon = egui::Image::new(ICON_PLUS)
                        //         .fit_to_exact_size(egui::vec2(icon_size, icon_size));

                        //     let button = egui::Button::image(icon);
                        //     if ui.add(button).clicked() {
                        //         self.display_modal = Some(DropshipModal::Options);
                        //     }
                        // }
                        {
                            let button = egui::Button::new("{{ add one }}")
                                .min_size(egui::vec2(ui.available_width(), 24.0))
                                .gap(8.);

                            if ui.add(button).clicked() {
                                let commands_tx = self.commands_tx.clone();
                                tokio::spawn(async move {
                                    // REVIEW with windows api we can make sure it's exactly overwatch.exe
                                    // ofn.lpstrFilter = TEXT("Overwatch.exe\0Overwatch.exe\0");
                                    // i cannot with rfd it seems..

                                    let file = rfd::AsyncFileDialog::new()
                                        // .add_filter("Overwatch", &["exe"])
                                        .add_filter("Overwatch.exe", &["exe"])
                                        .set_directory("/")
                                        // .set_title("find overwatch.exe")
                                        // .set_file_name("Overwatch.exe")
                                        .pick_file()
                                        .await;

                                    if let Some(file) = file {
                                        let path = file.path().to_path_buf();
                                        let _ = commands_tx
                                            .send(dropship::Command::AddExecutable { path });
                                    }
                                });
                            }
                        }
                    });
                });
            });
        });

        // path delegator
        if let Some(path) = &self.modal_manage_path {
            let mut should_close = false;

            let modal = egui::Modal::new(egui::Id::new("export_ips")).show(ui.ctx(), |ui| {
                ui.set_max_width(600.);
                ui.set_max_height(400.);

                Self::draw_path(&path, ui);

                ui.separator();

                {
                    let button = egui::Button::new("browse local files");
                    let button = ui.add_sized(egui::vec2(ui.available_width(), 16.0), button);

                    if button.clicked() {
                        #[cfg(target_os = "windows")]
                        if let Err(e) = std::process::Command::new("explorer")
                            .arg("/select,")
                            .arg(path)
                            .spawn()
                        {
                            log::error!("{}", e);
                        }

                        should_close = true;
                    }
                }

                if let Some(known_paths) = self.config.known_paths.as_mut() {
                    {
                        let button = egui::Button::new("forget this file");
                        let button = ui.add_sized(egui::vec2(ui.available_width(), 16.0), button);

                        if button.clicked() {
                            known_paths.remove(path);
                            match firewall::get_dropship_rule(path) {
                                Ok(rule) => {
                                    if let Some(rule) = rule {
                                        if let Err(e) = unsafe { rule.SetEnabled(false.into()) } {
                                            log::error!("{}", e);
                                        }
                                        let name = windows::core::BSTR::from("delete");
                                        if let Err(e) = unsafe { rule.SetName(&name) } {
                                            log::error!("{}", e);
                                        }

                                        if let Ok(rules) = unsafe { firewall::win::get_rules() } {
                                            if let Err(e) = unsafe { rules.Remove(&name) } {
                                                log::error!("{}", e);
                                            }
                                        }
                                    }
                                }
                                Err(e) => {
                                    log::error!("{}", e);
                                }
                            }

                            should_close = true;
                        }
                    }
                }
            });

            if modal.should_close() || should_close {
                self.modal_manage_path = None;
            }
        }
    }

    fn servers(&mut self, ui: &mut egui::Ui) {
        let mut selection_changed = false;

        let theme = self.get_theme(ui);

        {
            let mut new_blocked_servers = self.config.desired_blocked_servers.clone();

            egui::ScrollArea::vertical()
                .content_margin(egui::Margin {
                    right: 4 + 8, // gap + width + margin
                    top: 0,
                    left: 0,
                    bottom: 0,
                })
                .auto_shrink([false; 2])
                .show(ui, |ui| {
                    ui.vertical(|ui| {
                        selection_changed = components::server_list_item::server_list(
                            ui,
                            self.known_servers(),
                            &mut new_blocked_servers,
                            self.config.blocked_servers,
                            &self.pings,
                            //
                            theme,
                        );
                    });

                    if self.known_servers().is_empty() {
                        ui.label("no known servers");
                    }
                });

            if self.config.desired_blocked_servers.bits() != new_blocked_servers.bits() {
                self.config.desired_blocked_servers = new_blocked_servers;
            }
        }

        if selection_changed {
            self.apply_blocked_servers_to_firewall();
        }
    }

    fn tabs(&mut self, ui: &mut egui::Ui, frame: &mut eframe::Frame) {
        // let lines_light = egui::Image::new(egui::include_image!(
        //     "../assets/images/lines-light.png"
        // )).tint(egui::Color32::from_white_alpha(99)).corner_radius(egui::CornerRadius::same(8))
        // // .maintain_aspect_ratio(true)
        // ;
        // lines_light.paint_at(ui, ui.available_rect_before_wrap());

        // tab hotkeys
        if !ui.egui_wants_keyboard_input() {
            ui.ctx().input(|i| {
                for num in 1..=4 {
                    let key = match num {
                        1 => egui::Key::Num1,
                        2 => egui::Key::Num2,
                        3 => egui::Key::Num3,
                        4 => egui::Key::Num4,
                        _ => unreachable!(),
                    };

                    if i.key_pressed(key) {
                        self.tab = num - 1;
                    }
                }
            });
        }

        let theme = self.get_theme(ui);

        ui.scope(|ui| {
            ui.style_mut().spacing.item_spacing.y = 0.;

            ui.vertical(|ui| {
                ui.scope(|ui| {
                    let r = 8.0.into();
                    ui.style_mut().visuals.widgets.inactive.corner_radius = r;
                    ui.style_mut().visuals.widgets.hovered.corner_radius = r;
                    ui.style_mut().visuals.widgets.active.corner_radius = r;

                    ui.horizontal(|ui| {
                        {
                            let tabs = [
                                (Some(assets::ICON_MAPLE_LEAF), "welcome"),
                                (Some(assets::ICON_TERMINAL), "log"),
                                (Some(assets::ICON_HEART), "help"),
                                (Some(assets::ICON_GEARS), "options"),
                            ];
                            let len = tabs.len();

                            ui.horizontal(|ui| {
                                ui.spacing_mut().item_spacing = egui::Vec2::new(0., 0.);

                                tabs.into_iter().enumerate().for_each(|(i, (image, text))| {
                                    ui.scope(|ui| {
                                        // tab styling
                                        {
                                            if i != self.tab {
                                                // ui.style_mut().visuals.widgets.inactive.weak_bg_fill =
                                                //     egui::Color32::from_black_alpha(0);
                                                // ui.style_mut()
                                                //     .visuals
                                                //     .widgets
                                                //     .active
                                                //     .weak_bg_fill =
                                                //     egui::Color32::from_black_alpha(40);

                                                ui.style_mut().visuals.override_text_color =
                                                    Some(ui.style_mut().visuals.weak_text_color());
                                            } else {
                                                ui.style_mut()
                                                    .visuals
                                                    .widgets
                                                    .active
                                                    .weak_bg_fill =
                                                    visuals::from_theme_alpha(theme, 20);

                                                ui.style_mut()
                                                    .visuals
                                                    .widgets
                                                    .hovered
                                                    .weak_bg_fill =
                                                    visuals::from_theme_alpha(theme, 20);
                                            }
                                        }

                                        let corner_radius = match i {
                                            0 => egui::CornerRadius {
                                                nw: 8,
                                                ..Default::default()
                                            },
                                            x if x == len - 1 => egui::CornerRadius {
                                                ne: 8,
                                                ..Default::default()
                                            },
                                            _ => egui::CornerRadius::ZERO,
                                        };

                                        let btn = egui::Button::opt_image_and_text(
                                            image.map_or(None, |i| {
                                                Some(
                                                    egui::Image::new(i)
                                                        .fit_to_exact_size(egui::vec2(12.0, 12.0))
                                                        .tint(ui.style().visuals.text_color()),
                                                )
                                            }),
                                            Some(text.into()),
                                        )
                                        .gap(8.)
                                        .corner_radius(corner_radius)
                                        // .min_size(egui::vec2(90., 0.))
                                        ;

                                        if ui.add(btn).clicked() {
                                            self.tab = i;
                                        }
                                    });
                                });

                                ui.with_layout(
                                    egui::Layout::right_to_left(egui::Align::Center),
                                    |ui| {
                                        ui.scope(|ui| {
                                            if !self.notice_expanded {
                                                ui.style_mut()
                                                    .visuals
                                                    .widgets
                                                    .inactive
                                                    .weak_bg_fill =
                                                    visuals::from_theme_alpha(theme, 0);
                                                ui.style_mut()
                                                    .visuals
                                                    .widgets
                                                    .active
                                                    .weak_bg_fill =
                                                    visuals::from_theme_alpha(theme, 40);
                                                ui.style_mut()
                                                    .visuals
                                                    .widgets
                                                    .hovered
                                                    .weak_bg_fill =
                                                    visuals::from_theme_alpha(theme, 20);

                                                ui.style_mut().visuals.override_text_color =
                                                    Some(ui.style_mut().visuals.weak_text_color());
                                                ui.style_mut()
                                                    .visuals
                                                    .widgets
                                                    .active
                                                    .weak_bg_fill =
                                                    visuals::from_theme_alpha(theme, 20);

                                                ui.style_mut()
                                                    .visuals
                                                    .widgets
                                                    .hovered
                                                    .weak_bg_fill =
                                                    visuals::from_theme_alpha(theme, 20);
                                            }

                                            // let button =
                                            //     egui::Button::new(if !self.notice_expanded {
                                            //         "vvvv"
                                            //     } else {
                                            //         "close"
                                            //     })
                                            //     .corner_radius(egui::CornerRadius {
                                            //         ne: 8,
                                            //         nw: 8,
                                            //         se: if self.notice_expanded { 0 } else { 8 },
                                            //         sw: if self.notice_expanded { 0 } else { 8 },
                                            //     });

                                            // if ui.add(button).clicked() {
                                            //     self.notice_expanded = !self.notice_expanded;
                                            // }
                                        });
                                    },
                                );
                            });
                        }
                    });

                    egui::Frame::group(ui.style())
                        .outer_margin(egui::Margin {
                            top: 0,
                            bottom: 0,
                            left: 0,
                            right: 0,
                        })
                        // .outer_margin(egui::Margin::ZERO)
                        // .inner_margin(egui::Margin {
                        //     top: 0,
                        //     bottom: 0,
                        //     left: 0,
                        //     right: 0,
                        // })
                        .inner_margin(egui::Margin {
                            bottom: 4,
                            top: 4,
                            left: 0,
                            right: 6,
                        })
                        .fill(visuals::from_theme_alpha(theme, 20))
                        // .corner_radius(egui::CornerRadius::same(8))
                        .corner_radius(egui::CornerRadius {
                            ne: if self.notice_expanded { 0 } else { 8 },
                            nw: 0,
                            se: 8,
                            sw: 8,
                        })
                        // .corner_radius(egui::CornerRadius::ZERO)
                        .show(ui, |ui| {
                            ui.set_height(if !self.notice_expanded {
                                180.
                            } else {
                                ui.available_height()
                            });

                            //
                            // REVIEW disable this for full screen notice. could have a continue button / expand / detract button
                            // ui.set_max_height(140.);

                            egui::ScrollArea::vertical()
                                // .max_height(140.)
                                .stick_to_bottom(self.tab == TAB_LOG)
                                .id_salt(self.tab)
                                .content_margin(egui::Margin {
                                    // right: 4 + 8 + 8, // gap + width + margin
                                    right: 4 + if !self.notice_expanded { 8 } else { 0 } + 8, // gap + width + margin
                                    top: 8,
                                    left: 16,
                                    bottom: 8, // extra text padding at the bottom
                                    ..Default::default()
                                })
                                // REVIEW disable this for full screen notice. could have a continue button / expand / detract button
                                // .max_height(if !self.notice_expanded {
                                //     120.
                                // } else {
                                //     f32::INFINITY
                                // })
                                .auto_shrink([false; 2])
                                .show(ui, |ui| {
                                    ui.add_space(4.0);
                                    ui.vertical(|ui| match self.tab {
                                        // 1 => self.socials(ui),
                                        TAB_LOG => self.log(ui),
                                        2 => self.help_wizard(ui),
                                        3 => self.options(ui, frame),
                                        _ => self.notice(ui),
                                    });
                                });
                        });
                });
            });
        });
    }

    fn notice(&mut self, ui: &mut egui::Ui) {
        if let Some(notice) = self
            .cache
            .as_ref()
            .and_then(|x| x.cached_api_data.as_ref().and_then(|x| x.notices.last()))
        {
            ui.horizontal(|ui| {
                ui.heading(&notice.title);

                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    ui.label(&notice.date);
                });
            });

            ui.add_space(8.0);

            ui.label(&notice.paragraph);
        } else {
            // match self.dropship_api_data.state() {
            //     egui_async::StateWithData::Failed(error) => {
            //         ui.label(error.to_string());
            //     }

            //     _ => {
            //         ui.spinner();
            //     }
            // }
        }
    }

    fn log(&mut self, ui: &mut egui::Ui) {
        for record in &self.logs {
            let color = match record.level {
                log::Level::Error => ui.visuals().error_fg_color,
                log::Level::Warn => ui.visuals().warn_fg_color,
                log::Level::Info => crate::visuals::BATTLENET_BLUE,
                // log::Level::Debug => crate::visuals::HEX_BDB2FF,
                log::Level::Debug => egui::Color32::PURPLE,
                _ => ui.visuals().text_color(),
            };

            ui.horizontal(|ui| {
                ui.colored_label(color, record.level.to_string().to_ascii_lowercase());

                ui.add(egui::Label::new(&record.message).wrap())
                    .on_hover_ui_at_pointer(|ui| {
                        ui.label(format!("thread #{}", record.thread_id.as_u64().get()));
                        ui.add(
                            egui::Label::new(
                                chrono_humanize::HumanTime::from(record.time)
                                    .to_string()
                                    .to_ascii_lowercase(),
                            )
                            .wrap_mode(egui::TextWrapMode::Extend),
                        );
                    });
            });
        }
    }

    fn help_wizard(&mut self, ui: &mut egui::Ui) {
        ui.label("if something's not working, you can ask for help in the discord :3");
        ui.horizontal(|ui| {
            ui.label("  •");
            // ui.hyperlink_to("discord", dropship::DISCORD_INVITE_LINK);
            ui.hyperlink(dropship::DISCORD_INVITE_LINK);
        });

        ui.separator();

        ui.label("you could also post an issue on github");
        ui.horizontal(|ui| {
            ui.label("  •");
            // ui.hyperlink_to("discord", dropship::DISCORD_INVITE_LINK);
            ui.hyperlink(dropship::GITHUB_URI);
        });

        ui.separator();

        ui.label("desire a missing feature? please ask for it in the discord");
        ui.horizontal(|ui| {
            ui.label("  •");
            ui.hyperlink(dropship::DISCORD_INVITE_LINK);
        });
    }

    fn options(&mut self, ui: &mut egui::Ui, frame: &mut eframe::Frame) {
        ui.vertical(|ui| {
            // egui::Sides::new().show(ui, |ui| {}, |ui| {});

            let theme = self.get_theme(ui);

            egui::Panel::right("xx")
                .frame(
                    egui::Frame::default()
                        .outer_margin(egui::Margin::ZERO)
                        .inner_margin(egui::Margin::ZERO),
                )
                .resizable(false)
                .show(ui, |ui| {
                    // export ips
                    {
                        if ui.button("export blocked ips").clicked() {
                            self.export_ips_modal = true;
                        }

                        if self.export_ips_modal {
                            let modal = egui::Modal::new(egui::Id::new("export_ips")).show(
                                ui.ctx(),
                                |ui| {
                                    ui.set_max_width(400.);
                                    ui.set_max_height(400.);

                                    let ips = self
                                        .known_servers()
                                        .into_iter()
                                        .filter(|x| self.config.desired_blocked_servers.has(x))
                                        .map(|x| x.block.clone())
                                        .collect::<Vec<_>>();

                                    // ui.heading("ips");

                                    ui.horizontal(|ui| {
                                        components::server_list_item::server_list_indicators(
                                            ui,
                                            self.known_servers(),
                                            &self.config.desired_blocked_servers,
                                            self.config.blocked_servers,
                                            //
                                            theme,
                                        );
                                    });

                                    ui.separator();

                                    let plural = ips.len() != 1;
                                    ui.label(format!(
                                        "you have {} server{} blocked.",
                                        ips.len(),
                                        if plural { "s" } else { "" }
                                    ));

                                    if let Some(s) = self.known_servers().iter().find(|x| {
                                        self.config.blocked_servers.has(x)
                                            && x.token.starts_with("g")
                                    }) {
                                        ui.separator();

                                        ui.label(format!(
                                            "warning: {}'s ips often change.",
                                            &s.token
                                        ));

                                        // i do this weird layout so the formatter does not crash
                                        let x0 = "it's not a ";
                                        let x1 =
                                            "good idea to block the following servers manually";
                                        ui.label(x0.to_string() + x1);
                                    }

                                    if !ips.is_empty() {
                                        ui.separator();
                                        egui::ScrollArea::vertical()
                                            .content_margin(egui::Margin {
                                                right: 4 + 8, // gap + width + margin
                                                top: 0,
                                                left: 0,
                                                bottom: 0,
                                            })
                                            .auto_shrink([false, true])
                                            .show(ui, |ui| {
                                                ui.label(ips.join(","));
                                            });
                                    } else {
                                        // ui.separator();
                                        // ui.colored_label(ui.visuals().weak_text_color(), "none");
                                    }

                                    ui.separator();

                                    ui.scope(|ui| {
                                        if ips.is_empty() {
                                            ui.disable();
                                        }
                                        ui.vertical_centered(|ui| {
                                            let button = egui::Button::new("copy");
                                            let button = ui.add_sized(
                                                egui::vec2(ui.available_width(), 16.0),
                                                button,
                                            );

                                            if button.clicked() {
                                                ui.copy_text(ips.join(","));
                                            }
                                        });
                                    })
                                },
                            );

                            if modal.should_close() {
                                self.export_ips_modal = false;
                            }
                        }
                    }

                    ui.separator();

                    // persistence
                    {
                        if ui.button("wipe cache").clicked() {
                            {
                                // let cache = self.cache.clone();
                                // *self = Self::default();
                                self.config = DropshipConfig::default();

                                // self.welcomed = true;
                                // self.cache = cache;
                                self.cache = None;
                            }

                            // this is not necessary
                            {
                                if let Some(storage) = frame.storage_mut() {
                                    if let Ok(json) = serde_json::to_string(&self.config) {
                                        storage.set_string(eframe::APP_KEY, json);
                                    }
                                    storage.set_string(CACHE_KEY, "None".into());

                                    storage.flush();
                                }
                            }

                            if let Err(e) = firewall::delete_dropship_rules() {
                                log::error!("{}", e);
                            }

                            let _ = self
                                .commands_tx
                                .send(dropship::Command::UpdateConfigFromRemote);

                            self.apply_zoom(ui, self.config.zoom);
                            self.apply_theme(ui);

                            // ui.request_repaint();
                        }
                    }
                });

            // ui.separator();

            // // welcome
            // {
            //     if ui.button("show welcome page").clicked() {
            //         self.modal_welcome_page = Some(0);
            //     }

            //     ui.separator();

            //     ui.add(egui::Checkbox::new(
            //         &mut self.config.always_show_welcome,
            //         "always show welcome page when app is opened",
            //     ));
            // }

            // ui.separator();

            ui.separator();

            // window zoom

            {
                ui.horizontal(|ui| {
                    let window_scale = ui.add(
                        egui::DragValue::new(&mut self.config.zoom)
                            // .step_by(0.1)
                            .fixed_decimals(2)
                            // .range(0.5..=2.)
                            .range(0.75..=1.5)
                            .speed(0.02)
                            .update_while_editing(false),
                    );
                    if window_scale.drag_stopped() || window_scale.lost_focus() {
                        self.apply_zoom(ui, self.config.zoom);
                    }

                    ui.label("window size");
                });
            }

            ui.separator();

            // theme
            {
                self.theme_dropdown(ui);
            }

            ui.separator();

            {
                let mut is_checked = self.config.starting_tab == TAB_LOG;

                if ui
                    .checkbox(&mut is_checked, "open log when app starts")
                    .changed()
                {
                    self.config.starting_tab = if is_checked { TAB_LOG } else { 0 };
                }

                ui.separator();
            }

            ui.separator();
            if ui.link("windowsdefender://network").clicked() {
                std::process::Command::new("explorer.exe")
                    .arg("windowsdefender://network")
                    .spawn()
                    .ok();
            }

            ui.separator();

            if ui.link("wf.msc").clicked() {
                std::process::Command::new("mmc.exe")
                    .arg("wf.msc")
                    .spawn()
                    .ok();
            }
        });
    }

    fn welcome(&mut self, ui: &mut egui::Ui, page: u8) {
        //
        let trapped = !self.config.welcomed;

        let modal = egui::Modal::new(egui::Id::new("welcome")).show(ui.ctx(), |ui| {
            let mut last_page = false;

            ui.set_max_width(400.);
            ui.set_max_height(400.);

            match page {
                0 => {
                    ui.heading("overwatch server selector");
                    ui.label(
                        "this app grants you control over which overwatch servers you play on",
                    );

                    ui.separator();
                    ui.label("this app does *not*");
                    ui.indent("xd", |ui| {
                        ui.label("• modify any game files");
                        ui.label("• break blizzard terms of service");
                    });

                }
                1 => {
                    ui.heading("how it works");

                    ui.label("you can choose which server you want to play on by *blocking* the ones you don't");
                    ui.indent("xd4", |ui| {
                        ui.label("• you do not need to keep dropship open");
                        ui.label("• blocks persist until you undo them");
                    });
                }
                _ => {
                    // ui.heading("done");

                    ui.label("if something's not working, you can ask for help in the discord !!");
                    ui.horizontal(|ui| {
                        ui.label("  •");
                        // ui.hyperlink_to("discord", dropship::DISCORD_INVITE_LINK);
                        ui.hyperlink(dropship::DISCORD_INVITE_LINK);
                    });

                    if self.known_servers().is_empty() {
                        ui.separator();

                        ui.horizontal(|ui| {
                            ui.label("waiting for api data ");

                            ui.spinner();
                        });
                    }

                    ui.separator();

                    ui.label("choose a theme:");
                    self.theme_dropdown(ui);

                    // waiting for dropship data.
                    // do not allow continuing until there are known servers

                    last_page = true;
                }
            }

            ui.separator();

            let mut go_back = false;

            egui::Sides::new().show(
                ui,
                |ui| {
                    if page > 0 {
                        if ui.button(format!("< back")).clicked() {
                            // self.modal_welcome_page = Some(page - 1);
                            go_back = true;
                        }
                    } else {
                        {
                            if trapped {
                                let icon_size = 12.;

                                let icon = egui::Image::new(assets::ICON_POWER_OFF)
                                    .fit_to_exact_size(egui::vec2(icon_size, icon_size))
                                    .tint(ui.visuals().text_color());

                                let button = egui::Button::image_and_text(icon, "quit").gap(6.);
                                if ui.add(button).clicked() {
                                    ui.send_viewport_cmd(egui::ViewportCommand::Close);
                                }
                            }
                        }

                        // if trapped && ui.button("quit").clicked() {
                        //     ui.send_viewport_cmd(egui::ViewportCommand::Close);
                        // }
                    }
                },
                |ui| {
                    if !last_page {
                        if ui.button(format!("next >")).clicked() {
                            self.modal_welcome_page = Some(page + 1);
                        }
                    } else {
                        ui.scope(|ui| {
                            if self.known_servers().is_empty() {
                                ui.disable();
                            }

                            if ui
                                .button("get started")
                                .clicked()
                            {
                                self.config.welcomed = true;
                                self.modal_welcome_page = None;
                            }
                        });
                    }
                },
            );

            if go_back {
                self.modal_welcome_page = Some(page - 1);
            }
        });

        if !trapped && modal.should_close() {
            self.modal_welcome_page = None;
        }
    }

    fn updater(&mut self, ui: &mut egui::Ui) {
        if self.should_show_update_modal() {
            if let Some(update) = &self.update_available {
                let theme = self.get_theme(ui);

                let modal = egui::Modal::new(egui::Id::new("update")).show(ui.ctx(), |ui| {
                    ui.set_max_width(400.);
                    ui.set_max_height(400.);

                    match &self.installing_status {
                        update::UpdatingStatus::NotActive => {
                            ui.label(format!("version {} is available", update.version));
                            ui.colored_label(
                                ui.style().visuals.weak_text_color(),
                                format!(
                                    "{} • {:.2} mib • {} downloads",
                                    chrono_humanize::HumanTime::from(update.binary.updated_at)
                                        .to_string(),
                                    update.binary.size as f32 / 1_048_576.0,
                                    update.binary.download_count,
                                ),
                            );

                            {
                                ui.separator();
                                egui::Frame::group(ui.style())
                                    .fill(visuals::from_theme_alpha(theme, 20))
                                    .show(ui, |ui| {
                                        ui.set_width(ui.available_width());

                                        egui::ScrollArea::vertical()
                                            .max_height(128.)
                                            .content_margin(egui::Margin {
                                                right: 4 + 8, // gap + width + margin
                                                top: 0,
                                                left: 0,
                                                bottom: 0,
                                            })
                                            .auto_shrink([false, true])
                                            .show(ui, |ui| {
                                                ui.indent("..", |ui| {
                                                    ui.label(&update.description);
                                                })
                                            });
                                    });
                            }

                            ui.separator();

                            ui.vertical_centered(|ui| {
                                // let button = egui::Button::new("download");
                                let button = egui::Button::new("update");
                                let button =
                                    ui.add_sized(egui::vec2(ui.available_width(), 16.0), button);

                                if button.clicked() {
                                    // ui.copy_text(ips.join(","));

                                    // self.update_install.request(update::update(
                                    //     update.binary.browser_download_url.clone(),
                                    //     // self.update_progress.clone(),
                                    //     (&self.download_total_size, &self.downloaded_size),
                                    // ));

                                    let _ = self.commands_tx.send(
                                        dropship::Command::ApplicationUpdate {
                                            binary_download: update
                                                .binary
                                                .browser_download_url
                                                .clone(),
                                            download_total_size: self.download_total_size.clone(),
                                            downloaded_size: self.downloaded_size.clone(),
                                        },
                                    );
                                }
                            });
                        }
                        update::UpdatingStatus::Downloading => {
                            ui.label("downloading..");

                            ui.separator();

                            // let progress = match self.update_progress.lock() {
                            //     Ok(g) => *g,
                            //     Err(_) => 0.,
                            // };

                            let download_total_size =
                                self.download_total_size.load(atomic::Ordering::Relaxed);
                            match download_total_size {
                                0 => {
                                    ui.add(egui::ProgressBar::new(0.).show_percentage());
                                }
                                _ => {
                                    let downloaded_size =
                                        self.downloaded_size.load(atomic::Ordering::Relaxed);

                                    let progress =
                                        downloaded_size as f32 / download_total_size as f32;

                                    ui.add(
                                        egui::ProgressBar::new(progress)
                                            // .show_percentage()
                                            .text(format!(
                                                "{:.2}% ({:.2} mib / {:.2} mib)",
                                                progress * 100.,
                                                downloaded_size as f32 / 1_048_576.0,
                                                download_total_size as f32 / 1_048_576.0
                                            )),
                                    );
                                }
                            }
                        }
                        update::UpdatingStatus::Installed => {
                            let download_total_size =
                                self.download_total_size.load(atomic::Ordering::Relaxed);

                            ui.label("download complete");

                            ui.separator();

                            ui.add(
                                egui::ProgressBar::new(1.)
                                    // .show_percentage()
                                    .text(format!(
                                        "{:.2}% (downloaded {:.2} mib)",
                                        100.,
                                        download_total_size as f32 / 1_048_576.0,
                                    )),
                            );

                            ui.separator();

                            ui.vertical_centered(|ui| {
                                let button =
                                    egui::Button::new(format!("start v{}", update.version));
                                let button =
                                    ui.add_sized(egui::vec2(ui.available_width(), 16.0), button);

                                if button.clicked() {
                                    self.restart_requested = true;
                                    ui.send_viewport_cmd(egui::ViewportCommand::Close);
                                }
                            });
                        }
                        update::UpdatingStatus::Failed(e) => {
                            ui.label("error");
                            ui.separator();
                            ui.colored_label(ui.visuals().error_fg_color, e);

                            ui.vertical_centered(|ui| {
                                let button = egui::Button::new("close");
                                let button =
                                    ui.add_sized(egui::vec2(ui.available_width(), 16.0), button);

                                if button.clicked() {
                                    self.hide_update = true;
                                }
                            });
                        }
                    };
                });

                if modal.should_close() {
                    // prevent close while downloading
                    if !matches!(self.installing_status, update::UpdatingStatus::Downloading) {
                        // self.update_bind.abort();z
                        self.hide_update = true;
                    }
                }
            }
        }
    }

    fn should_show_update_modal(&self) -> bool {
        if self.update_available.is_some() {
            !self.hide_update
        } else {
            false
        }
    }

    fn theme_dropdown(&mut self, ui: &mut egui::Ui) {
        fn name(t: &Option<visuals::Theme>) -> String {
            if let Some(t) = t {
                t.as_ref().to_ascii_lowercase()
            } else {
                "same as pc".to_string()
            }
        }

        let before = self.config.theme;
        egui::ComboBox::from_label("theme")
            .selected_text(format!("{}", name(&self.config.theme)))
            .show_ui(ui, |ui| {
                ui.selectable_value(&mut self.config.theme, None, name(&None));
                ui.selectable_value(
                    &mut self.config.theme,
                    Some(visuals::Theme::Dark),
                    name(&Some(visuals::Theme::Dark)),
                );
                ui.selectable_value(
                    &mut self.config.theme,
                    Some(visuals::Theme::Light),
                    name(&Some(visuals::Theme::Light)),
                );
            });

        if self.config.theme != before {
            self.apply_theme(ui);
        }
    }
}
