use crate::{
    api::{self, KnownServer},
    assets, components,
    dropship::{self, startup_dispatch},
    firewall::{self, applications::ApplicationType},
    lang::{self, t, tf},
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
use tokio::sync::{
    Mutex,
    mpsc::{self, UnboundedReceiver, UnboundedSender},
};

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
    mini: bool,
    wfp_dynamic_session: bool, // do wfp blocks only apply when dropship is open?
    //
    disable_background_image: bool,
    /// none is the pc's language
    language: Option<lang::Lang>,
    /// has the onboarding tour been shown?
    toured: bool,
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
            mini: false,
            wfp_dynamic_session: false,
            //
            disable_background_image: false,
            language: None,
            toured: false,
        }
    }
}

const TAB_LOG: usize = 1;

/// horizontal row that reads in the current language's direction. wrapped in
/// `horizontal` because a bare `with_layout` expands vertically (and centers its
/// content) inside containers without a height, such as areas and scroll areas.
fn row<R>(ui: &mut egui::Ui, add: impl FnOnce(&mut egui::Ui) -> R) -> egui::InnerResponse<R> {
    if lang::rtl() {
        ui.horizontal(|ui| {
            ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), add)
                .inner
        })
    } else {
        ui.horizontal(add)
    }
}

/// the side text starts from in the current language
fn start_align() -> egui::Align {
    if lang::rtl() {
        egui::Align::RIGHT
    } else {
        egui::Align::LEFT
    }
}

/// a vertical stack aligned to the reading side
fn column(ui: &mut egui::Ui, add: impl FnOnce(&mut egui::Ui)) {
    ui.with_layout(egui::Layout::top_down(start_align()), add);
}

/// horizontal layout that runs against the reading direction (for things pushed
/// to the far side of a row, like the status message next to the version)
fn opposite_layout() -> egui::Layout {
    if lang::rtl() {
        egui::Layout::left_to_right(egui::Align::Center)
    } else {
        egui::Layout::right_to_left(egui::Align::Center)
    }
}

/// margins mirrored for right-to-left
fn margin(left: i8, right: i8, top: i8, bottom: i8) -> egui::Margin {
    if lang::rtl() {
        egui::Margin { left: right, right: left, top, bottom }
    } else {
        egui::Margin { left, right, top, bottom }
    }
}

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
    restart_requested: bool,
    tab: usize,
    pub(crate) loading: bool,
    pub(crate) pending_firewall_sync_when_game_is_closed: bool,
    pub(crate) legacy_cleanup_done: bool,
    // pub(crate) cached_lowest_ping_server: Option<KnownServer>,
    prev_system_theme: Option<egui::Theme>, //

    //
    pub(crate) suggesting_path: Option<PathBuf>,
    pub(crate) denied_paths: HashSet<PathBuf>,

    //
    wfp_connection: Arc<Mutex<Option<firewall::win::WfpConnection>>>,

    /// onboarding tour: current step
    tour: Option<usize>,
    /// where the widgets the tour points at were drawn this frame
    tour_rects: HashMap<&'static str, egui::Rect>,
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
                    log::warn!("{}", t("msg.config_failed"));
                    // log::warn!("didn't find a valid dropship config file");
                    Default::default()
                };

                // failable cache
                let cache = if let Some(value) =
                    eframe::get_value::<Option<ApiCache>>(storage, CACHE_KEY)
                {
                    value
                } else {
                    log::warn!("{}", t("msg.cache_failed"));
                    None
                };

                (config, cache)
            } else {
                log::warn!("{}", t("msg.no_config"));
                Default::default()
            }
        };

        // language first: the fonts and the theme depend on it
        lang::set(config.language.unwrap_or_else(lang::Lang::system));
        cc.egui_ctx.set_fonts(crate::visuals::fonts(lang::current()));

        {
            let size = egui::vec2(crate::APP_WIDTH, crate::APP_HEIGHT);
            cc.egui_ctx
                .send_viewport_cmd(egui::ViewportCommand::InnerSize(size));
            cc.egui_ctx.set_zoom_factor(config.zoom);
        }

        let (commands_tx, commands_rx) = mpsc::unbounded_channel::<dropship::Command>();
        let (events_tx, events_rx) = mpsc::unbounded_channel::<dropship::Event>();

        let wfp_connection = Arc::new(Mutex::new(None));

        dropship::start_processing_commands(
            commands_rx,
            events_tx,
            Some(cc.egui_ctx.clone()),
            wfp_connection.clone(),
        );

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
            restart_requested: false,
            tab: 0,
            loading: false,
            pending_firewall_sync_when_game_is_closed: false,
            legacy_cleanup_done: false,
            // cached_lowest_ping_server: None,
            prev_system_theme: cc.egui_ctx.system_theme(),
            //
            suggesting_path: None,
            denied_paths: HashSet::default(),

            //
            wfp_connection,

            tour: None,
            tour_rects: HashMap::new(),
        };

        {
            if !app.config.welcomed || app.config.always_show_welcome {
                app.modal_welcome_page = Some(0);
            }

            app.tab = app.config.starting_tab;

            if app.config.wfp_dynamic_session {
                app.config.blocked_servers = ServerSelection::none();
            }
        }

        app.apply_theme(&cc.egui_ctx);
        app.apply_wfp_session();
        app.apply_mini_mode(&cc.egui_ctx);

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

    fn get_theme(&self, ctx: &egui::Context) -> visuals::Theme {
        let theme = {
            if let Some(t) = self.config.theme {
                t
            } else {
                match ctx.system_theme() {
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

    fn apply_theme(&mut self, ctx: &egui::Context) {
        let theme = self.get_theme(ctx);

        ctx.all_styles_mut(move |style| crate::visuals::visuals(style, theme));
    }

    fn apply_wfp_session(&mut self) {
        let wfp_connection = self.wfp_connection.clone();
        let dynamic = self.config.wfp_dynamic_session;
        let commands_tx = self.commands_tx.clone();

        // tokio::task::spawn_blocking(move || {
        tokio::spawn(async move {
            //
            // wipe persistent filters for safety when in dynamic mode
            if dynamic {
                match firewall::win::WfpConnection::new(true) {
                    Ok(mut w) => {
                        match (|| -> std::io::Result<()> {
                            let transaction = wfp::Transaction::new(&mut w.handle)?;
                            firewall::win::delete_dropship_wfp(&transaction)?;
                            transaction.commit()?;
                            Ok(())
                        })() {
                            Ok(_) => {}
                            Err(e) => {
                                log::error!("{}", tf("msg.wfp_clean_failed", &[&e]));
                            }
                        }
                    }
                    Err(e) => {
                        log::error!("{}", tf("msg.wfp_connect_clean_failed", &[&e]));
                    }
                }
            }

            let mut guard = wfp_connection.lock().await;

            *guard = {
                match firewall::win::WfpConnection::new(!dynamic) {
                    Ok(w) => {
                        log::debug!("{}", tf("msg.wfp_connected", &[&dynamic]));
                        Some(w)
                    }
                    Err(e) => {
                        log::error!("{}", tf("msg.wfp_failed", &[&e]));
                        None
                    }
                }
            };

            let _ = commands_tx.send(dropship::Command::ForceApplyFirewallRequested);
        });
    }

    /// start the onboarding tour from the first step (needs the full window)
    fn start_tour(&mut self, ctx: &egui::Context) {
        self.config.mini = false;
        self.apply_mini_mode(ctx);
        self.tour = Some(0);
    }

    /// remember where a widget was drawn so the tour can point at it
    fn tour_mark(&mut self, key: &'static str, rect: egui::Rect) {
        self.tour_rects.insert(key, rect);
    }

    /// switch language at runtime: fonts, text size and accent colors follow it
    fn apply_language(&mut self, ctx: &egui::Context) {
        lang::set(self.config.language.unwrap_or_else(lang::Lang::system));
        ctx.set_fonts(crate::visuals::fonts(lang::current()));
        self.apply_theme(ctx);
    }

    fn apply_mini_mode(&self, ctx: &egui::Context) {
        if self.config.mini {
            ctx.send_viewport_cmd(egui::ViewportCommand::InnerSize(
                [crate::APP_MINI_WIDTH, crate::APP_HEIGHT].into(),
            ));
        } else {
            ctx.send_viewport_cmd(egui::ViewportCommand::InnerSize(
                [crate::APP_WIDTH, crate::APP_HEIGHT].into(),
            ));
        }
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
            log::info!("{}", t("msg.restart"));

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
            visuals::Theme::Light if visuals::green() => {
                egui::Color32::from_rgba_unmultiplied(240, 246, 242, 255).to_normalized_gamma_f32()
            }
            visuals::Theme::Dark if visuals::green() => {
                egui::Color32::from_rgba_unmultiplied(8, 22, 15, 255).to_normalized_gamma_f32()
            }
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
                log::debug!("{}", t("msg.theme_changed"));
                self.apply_theme(ctx);
            }
        }

        // cache previous frame's theme
        self.prev_system_theme = ctx.system_theme();
    }

    /// called each time the ui needs repainting, which may be many times per second.
    fn ui(&mut self, ui: &mut egui::Ui, frame: &mut eframe::Frame) {
        let esc_pressed: bool = ui.ctx().input(|i| i.key_pressed(egui::Key::Escape));

        if esc_pressed && self.tour.is_some() {
            // esc during the tour only skips it
            self.tour = None;
            self.config.toured = true;
        } else if esc_pressed
            && !ui.any_popup_open()
            && !self.export_ips_modal
            && self.modal_welcome_page.is_none()
            && !self.should_show_update_modal()
            && self.modal_manage_path.is_none()
        {
            ui.send_viewport_cmd(egui::ViewportCommand::Close);
        }

        let theme = self.get_theme(ui);

        if !self.config.disable_background_image {
            // let hero_bg = egui::Image::new(egui::include_image!("../assets/images/hero-bg.png")).tint();
            let hero_bg = {
                match (self.get_theme(ui), visuals::green()) {
                    (visuals::Theme::Light, false) => {
                        egui::Image::new(egui::include_image!("../assets/images/hero-bg.png"))
                    }
                    (visuals::Theme::Dark, false) => {
                        egui::Image::new(egui::include_image!("../assets/images/hero-bg_dark.png"))
                    }
                    (visuals::Theme::Light, true) => {
                        egui::Image::new(egui::include_image!("../assets/images/hero-bg-green.png"))
                    }
                    (visuals::Theme::Dark, true) => egui::Image::new(egui::include_image!(
                        "../assets/images/hero-bg-green_dark.png"
                    )),
                }
                .show_loading_spinner(false)
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
        }

        // footer
        egui::Panel::bottom("bottom_panel")
            .frame(
                egui::Frame::default()
                    // .outer_margin(egui::Margin::symmetric(16, 16))
                    .outer_margin(margin(16, 0, 16 + 16, 16))
                    .inner_margin(egui::Margin::ZERO),
            )
            .show(ui, |ui| {
                let rtl = lang::rtl();
                let mini = self.config.mini;

                // credits on the reading side, mouse/keyboard legend on the far side
                let credits = |ui: &mut egui::Ui| {
                    ui.with_layout(egui::Layout::top_down(start_align()), |ui| {
                        ui.hyperlink_to(
                            egui::RichText::new(t("footer.source")).small(),
                            dropship::GITHUB_URI,
                        )
                        .on_hover_text_at_pointer(dropship::GITHUB_URI);

                        ui.label(t("footer.written_by"));
                    });
                };

                let legend = |ui: &mut egui::Ui| {
                    ui.scope(|ui| {
                        ui.disable();

                        ui.add(egui::Button::new("").fill(egui::Color32::TRANSPARENT));

                        // a legend entry: the label goes on the reading side of its widget
                        let entry = |ui: &mut egui::Ui, label: &str, widget: &dyn Fn(&mut egui::Ui)| {
                            if rtl {
                                widget(ui);
                                ui.label(label);
                            } else {
                                ui.label(label);
                                widget(ui);
                            }
                        };
                        let icon = |ui: &mut egui::Ui, source: egui::ImageSource<'static>| {
                            let icon = egui::Image::new(source)
                                .fit_to_exact_size(egui::vec2(16., 16.))
                                .tint(ui.visuals().text_color());
                            ui.add(egui::Button::image(icon));
                        };

                        let others = |ui: &mut egui::Ui| entry(ui, t("footer.toggle_others"), &|ui| icon(ui, assets::ICON_M2));
                        let toggle = |ui: &mut egui::Ui| entry(ui, t("footer.toggle"), &|ui| icon(ui, assets::ICON_M1));
                        let close = |ui: &mut egui::Ui| {
                            entry(ui, t("footer.close"), &|ui| {
                                if ui.button("esc").clicked() {
                                    ui.send_viewport_cmd(egui::ViewportCommand::Close);
                                };
                            })
                        };

                        // the layout runs from the far edge inwards, so the order flips with the direction
                        if rtl {
                            close(ui);
                            if !mini {
                                ui.separator();
                                toggle(ui);
                                ui.separator();
                                others(ui);
                            }
                        } else {
                            if !mini {
                                others(ui);
                                ui.separator();
                                toggle(ui);
                                ui.separator();
                            }
                            close(ui);
                        }
                    });
                };

                if rtl {
                    egui::Sides::new().show(ui, legend, credits);
                } else {
                    egui::Sides::new().show(ui, credits, legend);
                }
                self.tour_mark("footer", ui.min_rect());
            });

        egui::Panel::top("top_panel")
            .frame(egui::Frame::default().outer_margin(egui::Margin {
                top: 16,
                left: 16,
                right: 16,
                bottom: 16,
            }))
            .show(ui, |ui| {
                let r = row(ui, |ui| {
                    // ui.label("<version />");
                    ui.label(format!("v{}", env!("CARGO_PKG_VERSION")));

                    #[cfg(debug_assertions)]
                    egui::warn_if_debug_build(ui);

                    ui.with_layout(opposite_layout(), |ui| {
                        // ui.label("@stormyy_ow");
                        self.stat(ui);
                    });
                });
                self.tour_mark("status", r.response.rect);
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
        // the server list sits on the far side of the reading direction
        if lang::rtl() { egui::Panel::left("side_panel") } else { egui::Panel::right("side_panel") }
            // .frame(egui::Frame::default())
            .frame(egui::Frame::default().outer_margin(egui::Margin::symmetric(8, 0)).inner_margin(margin(12, 0, 0, 0)))
            .exact_size(crate::APP_MINI_WIDTH)
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
                            let button = egui::Button::new(t("side.disable"));
                            let button =
                                ui.add_sized(egui::vec2(ui.available_width() - 8. - 4., 16.0), button)
                                    .on_hover_text(t("side.disable_hint"));

                            self.tour_mark("disable", button.rect);

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

                        row(ui, |ui| {
                            let mut toggle = |ui: &mut egui::Ui| {
                                let icon_size = 12.;

                                let icon = egui::Image::new(assets::ANGLES_RIGHT)
                                    .fit_to_exact_size(egui::vec2(icon_size, icon_size))
                                    .tint(ui.visuals().text_color()).rotate( if !self.config.mini { std::f32::consts::PI } else { 0. }, egui::Vec2::splat(0.5));

                                let button = egui::Button::image(icon);

                                let hint = if !self.config.mini { t("side.hide_details") } else { t("side.show_details") };

                                let button = ui.add(button).on_hover_text_at_pointer(hint);
                                self.tour_mark("mini", button.rect);

                                if button.clicked() {
                                    self.config.mini = !self.config.mini;

                                    self.apply_mini_mode(ui);
                                };
                            };

                            if lang::rtl() {
                                // label on the reading side, toggle pushed to the far edge
                                ui.label(t("side.play_on"));
                                ui.with_layout(egui::Layout::left_to_right(egui::Align::Center), |ui| toggle(ui));
                            } else {
                                toggle(ui);
                                ui.label(t("side.play_on"));
                            }
                        });

                        ui.separator();

                        let servers_top = ui.cursor().min;
                        self.servers(ui);
                        self.tour_mark("servers", egui::Rect::from_min_max(servers_top, ui.max_rect().max));
                    });

            });

        // main
        if !self.config.mini {
            egui::CentralPanel::default()
            .frame(
                egui::Frame::default()
                    // .outer_margin(egui::Margin::symmetric(32, 0))
                    // .inner_margin(egui::Margin::ZERO),
                    .outer_margin(margin(32, 16, 0, 0))
                    .inner_margin(egui::Margin::ZERO),
            )
            .show(ui, |ui| {
              column(ui, |ui| {
                let games_top = ui.cursor().min;
                ui.vertical(|ui| {

                    ui.heading({
                        match &self.config.known_paths.as_ref().map_or(0, |x| x.len()) {
                            0 => t("main.no_games"),
                            1 => t("main.this_game"),
                            _ => t("main.these_games"),
                        }
                    });

                    // ui.disable();
                    self.applications(ui);
                });
                self.tour_mark("games", egui::Rect::from_min_max(games_top, egui::pos2(ui.max_rect().max.x, ui.cursor().min.y)));

                ui.heading(t("main.will_play_on"));

                let stars = row(ui, |ui| {
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
                    ui.label(tf("main.blocked_count", &[&blocked]));
                });
                self.tour_mark("stars", stars.response.rect);

                ui.separator();

                ui.label(t("main.applied_note"));

                // if let Some(lowest_ping_server) = &self.cached_lowest_ping_server {
                if let Some(lowest_ping_server) = self.get_most_likely_to_play_on() {
                    // ui.separator();
                    ui.label(tf("main.most_likely", &[&lowest_ping_server.title, &lowest_ping_server.token]));
                }

                ui.separator();


                egui::Panel::bottom("tabs")
                    .frame(egui::Frame::default().outer_margin(egui::Margin::same(0)))
                    .show(ui, |ui| {
                        self.tabs(ui, frame);
                    });
              });
            });
        }

        if let Some(page) = self.modal_welcome_page {
            self.welcome(ui, page);
        }

        self.updater(ui);

        if self.suggesting_path.is_some() {
            self.suggest_path(ui);
        }

        if let Some(step) = self.tour {
            self.tour_overlay(ui, step);
        }

        #[cfg(debug_assertions)]
        // testing
        {
            // ui.set_debug_on_hover(true);

            // let r = egui::Rect::from_min_max(egui::pos2(563.3, 425.3), egui::pos2(641.3, 448.7));
            // ui.ctx().debug_painter().rect_stroke(
            //     r,
            //     0.0,
            //     (2.0, egui::Color32::RED),
            //     egui::StrokeKind::Middle,
            // );
        }

        #[cfg(debug_assertions)]
        if ui.button("force apply").clicked() {
            self._force_apply_blocked_servers_to_firewall();
        }
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

    pub fn _force_apply_blocked_servers_to_firewall(&mut self) {
        Self::_apply_blocked_servers_to_firewall(
            &self.config.desired_blocked_servers,
            self.known_servers(),
            &self.config.known_paths,
            &self.commands_tx,
        );
    }

    pub fn apply_blocked_servers_to_firewall(&mut self) {
        if self.game_open {
            log::warn!("{}", t("msg.close_game"));
            self.pending_firewall_sync_when_game_is_closed = true;
        } else {
            self._force_apply_blocked_servers_to_firewall();
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

                    self.config.mini = false;
                    self.apply_mini_mode(ui);
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
                            let button = egui::Button::new(t("main.add_one"))
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
                    let button = egui::Button::new(t("main.browse"));
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
                        let button = egui::Button::new(t("main.forget"));
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
                        ui.label(t("side.no_servers"));
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

        // tab hotkeys (off during the tour, which drives the tabs itself)
        if !ui.egui_wants_keyboard_input() && self.tour.is_none() {
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

                    row(ui, |ui| {
                        {
                            let tabs = [
                                (Some(assets::ICON_MAPLE_LEAF), t("tab.welcome"), "tab_notices"),
                                (Some(assets::ICON_TERMINAL), t("tab.log"), "tab_log"),
                                (Some(assets::ICON_HEART), t("tab.help"), "tab_help"),
                                (Some(assets::ICON_GEARS), t("tab.options"), "tab_options"),
                            ];
                            let len = tabs.len();
                            let rtl = lang::rtl();

                            row(ui, |ui| {
                                ui.spacing_mut().item_spacing = egui::Vec2::new(0., 0.);

                                tabs.into_iter().enumerate().for_each(|(i, (image, text, tour_key))| {
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

                                        // round the outer corners of the first and last tab
                                        let corner_radius = match i {
                                            0 => egui::CornerRadius {
                                                nw: if rtl { 0 } else { 8 },
                                                ne: if rtl { 8 } else { 0 },
                                                ..Default::default()
                                            },
                                            x if x == len - 1 => egui::CornerRadius {
                                                ne: if rtl { 0 } else { 8 },
                                                nw: if rtl { 8 } else { 0 },
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

                                        let btn = ui.add(btn);
                                        self.tour_mark(tour_key, btn.rect);
                                        if btn.clicked() {
                                            self.tab = i;
                                        }
                                    });
                                });
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
                        .inner_margin(margin(0, 6, 4, 4))
                        .fill(visuals::from_theme_alpha(theme, 20))
                        // .corner_radius(egui::CornerRadius::same(8))
                        // the corner under the first tab stays square
                        .corner_radius(egui::CornerRadius {
                            ne: if lang::rtl() { 0 } else { 8 },
                            nw: if lang::rtl() { 8 } else { 0 },
                            se: 8,
                            sw: 8,
                        })
                        // .corner_radius(egui::CornerRadius::ZERO)
                        .show(ui, |ui| {
                            ui.set_height(180.);

                            //
                            // REVIEW disable this for full screen notice. could have a continue button / expand / detract button
                            // ui.set_max_height(140.);

                            egui::ScrollArea::vertical()
                                // .max_height(140.)
                                .stick_to_bottom(self.tab == TAB_LOG)
                                .id_salt(self.tab)
                                // scrollbar side: gap + width + margin; text side: 16
                                .content_margin(margin(16, 4 + 8 + 8, 8, 8))
                                // REVIEW disable this for full screen notice. could have a continue button / expand / detract button
                                // .max_height(if !self.notice_expanded {
                                //     120.
                                // } else {
                                //     f32::INFINITY
                                // })
                                .auto_shrink([false; 2])
                                .show(ui, |ui| {
                                    ui.add_space(4.0);
                                    column(ui, |ui| match self.tab {
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
            row(ui, |ui| {
                ui.heading(&notice.title);

                ui.with_layout(opposite_layout(), |ui| {
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

            row(ui, |ui| {
                let level = match record.level {
                    log::Level::Error => t("log.error"),
                    log::Level::Warn => t("log.warn"),
                    log::Level::Info => t("log.info"),
                    log::Level::Debug => t("log.debug"),
                    log::Level::Trace => t("log.trace"),
                };
                ui.colored_label(color, level);

                ui.add(egui::Label::new(&record.message).wrap())
                    .on_hover_ui_at_pointer(|ui| {
                        ui.label(tf("log.thread", &[&record.thread_id.as_u64().get()]));
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
        ui.label(t("help.discord"));
        row(ui, |ui| {
            ui.label("  •");
            // ui.hyperlink_to("discord", dropship::DISCORD_INVITE_LINK);
            ui.hyperlink(dropship::DISCORD_INVITE_LINK);
        });

        ui.separator();

        ui.label(t("help.github"));
        row(ui, |ui| {
            ui.label("  •");
            // ui.hyperlink_to("discord", dropship::DISCORD_INVITE_LINK);
            ui.hyperlink(dropship::GITHUB_URI);
        });

        ui.separator();

        ui.label(t("help.feature"));
        row(ui, |ui| {
            ui.label("  •");
            ui.hyperlink(dropship::DISCORD_INVITE_LINK);
        });

        ui.separator();

        if ui.button(t("help.tour")).clicked() {
            self.start_tour(ui.ctx());
        }
    }

    fn options(&mut self, ui: &mut egui::Ui, frame: &mut eframe::Frame) {
        ui.vertical(|ui| {
            // egui::Sides::new().show(ui, |ui| {}, |ui| {});

            let theme = self.get_theme(ui);

            if lang::rtl() { egui::Panel::left("xx") } else { egui::Panel::right("xx") }
                .frame(
                    egui::Frame::default()
                        .outer_margin(egui::Margin::ZERO)
                        .inner_margin(egui::Margin::ZERO),
                )
                .exact_size(290.)
                .resizable(false)
                .show(ui, |ui| {
                  column(ui, |ui| {
                    // export ips
                    {
                        if ui.button(t("options.export")).clicked() {
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

                                    ui.label(match ips.len() {
                                        0 => t("options.export_count.0").to_string(),
                                        1 => t("options.export_count.1").to_string(),
                                        n => tf("options.export_count.n", &[&n]),
                                    });

                                    if let Some(s) = self.known_servers().iter().find(|x| {
                                        self.config.blocked_servers.has(x)
                                            && x.token.starts_with("g")
                                    }) {
                                        ui.separator();

                                        ui.label(tf("options.export_warn", &[&s.token]));
                                        ui.label(t("options.export_warn2"));
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
                                            let button = egui::Button::new(t("options.copy"));
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
                        if ui.button(t("options.wipe")).clicked() {
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
                            self.apply_mini_mode(ui);

                            // ui.request_repaint();
                        }
                    }

                    ui.separator();
                    if ui.link(t("options.reset_firewall")).clicked() {
                        match unsafe { firewall::win::reset_global_windows_firewall() } {
                            Ok(_) => {
                                log::debug!("{}", t("msg.firewall_reset"));
                            }
                            Err(e) => {
                                log::error!("{}", e.to_string());
                            }
                        }
                    }

                    ui.separator();
                    if ui.link(t("options.flush_dns")).clicked() {
                        unsafe { firewall::win::flush_dns() };
                    }
                  });
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

          column(ui, |ui| {
            ui.separator();

            // window zoom

            {
                row(ui, |ui| {
                    let mut drag = |ui: &mut egui::Ui| {
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
                    };

                    // label on the reading side of the widget
                    if lang::rtl() {
                        ui.label(t("options.window_size"));
                        drag(ui);
                    } else {
                        drag(ui);
                        ui.label(t("options.window_size"));
                    }
                });
            }

            ui.separator();

            // theme
            {
                self.theme_dropdown(ui);
            }

            ui.separator();

            // language
            {
                self.language_dropdown(ui);
            }

            ui.separator();

            // wfp dynamic
            {
                self.dynamic_wfp_dropdown(ui);
            }

            ui.separator();

            {
                let mut is_checked = self.config.starting_tab == TAB_LOG;

                if ui
                    .checkbox(&mut is_checked, t("options.open_log"))
                    .changed()
                {
                    self.config.starting_tab = if is_checked { TAB_LOG } else { 0 };
                }

                ui.separator();
            }

            {
                ui.checkbox(
                    &mut self.config.disable_background_image,
                    t("options.no_background"),
                );

                ui.separator();
            }

            {
                if ui.button(t("options.tour")).clicked() {
                    self.start_tour(ui.ctx());
                }

                ui.separator();
            }
          });

            // ui.separator();
            // if ui.link("windowsdefender://network").clicked() {
            //     std::process::Command::new("explorer.exe")
            //         .arg("windowsdefender://network")
            //         .spawn()
            //         .ok();
            // }

            // ui.separator();
            // if ui.link("wf.msc").clicked() {
            //     std::process::Command::new("mmc.exe")
            //         .arg("wf.msc")
            //         .spawn()
            //         .ok();
            // }
        });
    }

    fn welcome(&mut self, ui: &mut egui::Ui, page: u8) {
        //
        let trapped = !self.config.welcomed;

        let modal = egui::Modal::new(egui::Id::new("welcome")).show(ui.ctx(), |ui| {
            let mut last_page = false;

            ui.set_max_width(400.);
            ui.set_max_height(400.);

            column(ui, |ui| {
            match page {
                0 => {
                    ui.heading(t("welcome.title"));
                    ui.label(t("welcome.intro"));

                    ui.separator();
                    ui.label(t("welcome.does_not"));
                    ui.indent("xd", |ui| {
                        ui.label(t("welcome.no_modify"));
                        ui.label(t("welcome.no_tos"));
                    });

                }
                1 => {
                    ui.heading(t("welcome.how"));

                    ui.label(t("welcome.how_text"));
                    ui.indent("xd4", |ui| {
                        ui.label(t("welcome.no_keep_open"));
                        ui.label(t("welcome.persist"));
                    });
                }
                _ => {
                    // ui.heading("done");

                    ui.label(t("welcome.discord"));
                    row(ui, |ui| {
                        ui.label("  •");
                        // ui.hyperlink_to("discord", dropship::DISCORD_INVITE_LINK);
                        ui.hyperlink(dropship::DISCORD_INVITE_LINK);
                    });

                    if self.known_servers().is_empty() {
                        ui.separator();

                        row(ui, |ui| {
                            ui.label(t("welcome.waiting"));

                            ui.spinner();
                        });
                    }

                    ui.separator();

                    ui.label(t("welcome.choose_theme"));
                    self.theme_dropdown(ui);
                    self.language_dropdown(ui);

                    // waiting for dropship data.
                    // do not allow continuing until there are known servers

                    last_page = true;
                }
            }
            });

            ui.separator();

            // enter also advances, so the same key press must not also drive the tour that starts this frame
            let enter = ui.input_mut(|i| i.consume_key(egui::Modifiers::NONE, egui::Key::Enter));
            let servers_known = !self.known_servers().is_empty();

            let mut go_back = false;
            let mut go_next = false;
            let mut start = false;
            let mut quit = false;

            // back/quit on the reading side, next/start on the far side
            let back = |ui: &mut egui::Ui| {
                if page > 0 {
                    if ui.button(t("welcome.back")).clicked() {
                        go_back = true;
                    }
                } else if trapped {
                    let icon_size = 12.;

                    let icon = egui::Image::new(assets::ICON_POWER_OFF)
                        .fit_to_exact_size(egui::vec2(icon_size, icon_size))
                        .tint(ui.visuals().text_color());

                    let button = egui::Button::image_and_text(icon, t("welcome.quit")).gap(6.);
                    if ui.add(button).clicked() {
                        quit = true;
                    }
                }
            };
            let next = |ui: &mut egui::Ui| {
                if !last_page {
                    if ui.button(t("welcome.next")).clicked() || enter {
                        go_next = true;
                    }
                } else {
                    ui.scope(|ui| {
                        if !servers_known {
                            ui.disable();
                        }

                        if (ui.button(t("welcome.start")).clicked() || enter) && servers_known {
                            start = true;
                        }
                    });
                }
            };

            if lang::rtl() {
                egui::Sides::new().show(ui, next, back);
            } else {
                egui::Sides::new().show(ui, back, next);
            }

            if quit {
                ui.send_viewport_cmd(egui::ViewportCommand::Close);
            }
            if go_back {
                self.modal_welcome_page = Some(page - 1);
            }
            if go_next {
                self.modal_welcome_page = Some(page + 1);
            }
            if start {
                self.config.welcomed = true;
                self.modal_welcome_page = None;
                if !self.config.toured {
                    self.start_tour(ui.ctx());
                }
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

                    column(ui, |ui|
                    match &self.installing_status {
                        update::UpdatingStatus::NotActive => {
                            ui.label(tf("update.available", &[&update.version]));
                            ui.colored_label(
                                ui.style().visuals.weak_text_color(),
                                tf(
                                    "update.meta",
                                    &[
                                        &chrono_humanize::HumanTime::from(update.binary.updated_at)
                                            .to_string(),
                                        &format!("{:.2}", update.binary.size as f32 / 1_048_576.0),
                                        &update.binary.download_count,
                                    ],
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
                                let button = egui::Button::new(t("update.update"));
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
                            ui.label(t("update.downloading"));

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
                                            .text(tf(
                                                "update.progress",
                                                &[
                                                    &format!("{:.2}", progress * 100.),
                                                    &format!("{:.2}", downloaded_size as f32 / 1_048_576.0),
                                                    &format!("{:.2}", download_total_size as f32 / 1_048_576.0),
                                                ],
                                            )),
                                    );
                                }
                            }
                        }
                        update::UpdatingStatus::Installed => {
                            let download_total_size =
                                self.download_total_size.load(atomic::Ordering::Relaxed);

                            ui.label(t("update.complete"));

                            ui.separator();

                            ui.add(
                                egui::ProgressBar::new(1.)
                                    // .show_percentage()
                                    .text(tf(
                                        "update.downloaded",
                                        &[&"100.00", &format!("{:.2}", download_total_size as f32 / 1_048_576.0)],
                                    )),
                            );

                            ui.separator();

                            ui.vertical_centered(|ui| {
                                let button =
                                    egui::Button::new(tf("update.start", &[&update.version]));
                                let button =
                                    ui.add_sized(egui::vec2(ui.available_width(), 16.0), button);

                                if button.clicked() {
                                    self.restart_requested = true;
                                    ui.send_viewport_cmd(egui::ViewportCommand::Close);
                                }
                            });
                        }
                        update::UpdatingStatus::Failed(e) => {
                            ui.label(t("update.error"));
                            ui.separator();
                            ui.colored_label(ui.visuals().error_fg_color, e);

                            ui.vertical_centered(|ui| {
                                let button = egui::Button::new(t("update.close"));
                                let button =
                                    ui.add_sized(egui::vec2(ui.available_width(), 16.0), button);

                                if button.clicked() {
                                    self.hide_update = true;
                                }
                            });
                        }
                    });
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

    fn suggest_path(&mut self, ui: &mut egui::Ui) {
        let mut denied = false;

        if let Some(path) = &self.suggesting_path {
            //
            let mut should_close = false;

            let modal = egui::Modal::new(egui::Id::new("update")).show(ui.ctx(), |ui| {
                ui.set_max_width(400.);
                ui.set_max_height(400.);

                column(ui, |ui| {
                    ui.heading(t("suggest.title"));
                    ui.label(t("suggest.text"));
                });

                ui.separator();

                Self::draw_path(path, ui);

                ui.separator();

                {
                    let button = egui::Button::new(t("suggest.add"));
                    let button = ui.add_sized(egui::vec2(ui.available_width(), 16.0), button);

                    if button.clicked() {
                        // events_tx.send(Event::AddedExecutable(path))
                        let path = path.clone();
                        let _ = self
                            .commands_tx
                            .send(dropship::Command::AddExecutable { path });

                        should_close = true;
                    }
                }

                ui.scope(|ui| {
                    {
                        let theme = self.get_theme(ui);

                        ui.style_mut().visuals.widgets.inactive.weak_bg_fill =
                            visuals::from_theme_alpha(theme, 0);
                        ui.style_mut().visuals.widgets.active.weak_bg_fill =
                            visuals::from_theme_alpha(theme, 40);
                        ui.style_mut().visuals.widgets.hovered.weak_bg_fill =
                            visuals::from_theme_alpha(theme, 20);

                        ui.style_mut().visuals.override_text_color =
                            Some(ui.style_mut().visuals.weak_text_color());
                    }

                    let button = egui::Button::new(t("suggest.ignore"));
                    let button = ui.add_sized(egui::vec2(ui.available_width(), 16.0), button);

                    if button.clicked() {
                        should_close = true;
                        denied = true;
                    }
                });
            });

            if modal.should_close() || should_close {
                if denied {
                    self.denied_paths.insert(path.to_owned());
                }
                self.suggesting_path = None;
            }
        }
    }

    /// a combo box with its label on the reading side (like `ComboBox::from_label`,
    /// but mirrored for right-to-left)
    fn labeled_combo(ui: &mut egui::Ui, id: &str, label: &str, selected: &str, add: impl FnOnce(&mut egui::Ui)) {
        row(ui, |ui| {
            let combo = |ui: &mut egui::Ui| {
                egui::ComboBox::from_id_salt(id)
                    .selected_text(selected)
                    .show_ui(ui, add);
            };
            if lang::rtl() {
                ui.label(label);
                combo(ui);
            } else {
                combo(ui);
                ui.label(label);
            }
        });
    }

    fn theme_dropdown(&mut self, ui: &mut egui::Ui) {
        fn name(theme: &Option<visuals::Theme>) -> &'static str {
            match theme {
                Some(visuals::Theme::Light) => t("options.theme.light"),
                Some(visuals::Theme::Dark) => t("options.theme.dark"),
                None => t("lang.auto"),
            }
        }

        let before = self.config.theme;
        let theme = &mut self.config.theme;
        Self::labeled_combo(ui, "theme", t("options.theme"), name(theme), |ui| {
            ui.selectable_value(theme, None, name(&None));
            ui.selectable_value(theme, Some(visuals::Theme::Dark), name(&Some(visuals::Theme::Dark)));
            ui.selectable_value(theme, Some(visuals::Theme::Light), name(&Some(visuals::Theme::Light)));
        });

        if self.config.theme != before {
            self.apply_theme(ui);
        }
    }

    fn language_dropdown(&mut self, ui: &mut egui::Ui) {
        fn name(language: &Option<lang::Lang>) -> &'static str {
            match language {
                Some(l) => l.name(),
                None => t("lang.auto"),
            }
        }

        let before = self.config.language;
        let language = &mut self.config.language;
        Self::labeled_combo(ui, "language", t("options.language"), name(language), |ui| {
            ui.selectable_value(language, None, name(&None));
            for l in lang::Lang::ALL {
                ui.selectable_value(language, Some(l), l.name());
            }
        });

        if self.config.language != before {
            self.apply_language(ui);
        }
    }

    fn dynamic_wfp_dropdown(&mut self, ui: &mut egui::Ui) {
        fn name(dynamic: bool) -> &'static str {
            if dynamic {
                t("options.block.dynamic")
            } else {
                t("options.block.always")
            }
        }

        let before = self.config.wfp_dynamic_session;
        let dynamic = &mut self.config.wfp_dynamic_session;
        Self::labeled_combo(ui, "block_servers", t("options.block"), name(*dynamic), |ui| {
            ui.selectable_value(dynamic, false, name(false));
            ui.selectable_value(dynamic, true, name(true));
        });

        if self.config.wfp_dynamic_session != before {
            self.apply_wfp_session();
        }
    }
}

// ---------------------------------------------------------------------------
// onboarding tour: dims the window, outlines one widget at a time and explains it
// ---------------------------------------------------------------------------

/// (widget key, tab to open, title key, body key)
const TOUR_STEPS: &[(&str, Option<usize>, &str, &str)] = &[
    ("servers", None, "tour.servers.title", "tour.servers.body"),
    ("mini", None, "tour.mini.title", "tour.mini.body"),
    ("disable", None, "tour.disable.title", "tour.disable.body"),
    ("games", None, "tour.games.title", "tour.games.body"),
    ("stars", None, "tour.stars.title", "tour.stars.body"),
    ("tab_notices", Some(0), "tour.notices.title", "tour.notices.body"),
    ("tab_log", Some(1), "tour.log.title", "tour.log.body"),
    ("tab_help", Some(2), "tour.help.title", "tour.help.body"),
    ("tab_options", Some(3), "tour.options.title", "tour.options.body"),
    ("status", None, "tour.status.title", "tour.status.body"),
    ("footer", None, "tour.footer.title", "tour.footer.body"),
];

impl TemplateApp {
    fn tour_overlay(&mut self, ui: &mut egui::Ui, step: usize) {
        let Some(&(key, tab, title, body)) = TOUR_STEPS.get(step) else {
            self.tour = None;
            self.config.toured = true;
            return;
        };

        // open the tab this step talks about
        if let Some(tab) = tab {
            self.tab = tab;
        }

        let ctx = ui.ctx().clone();
        let screen = ctx.viewport_rect();
        let target = self
            .tour_rects
            .get(key)
            .copied()
            .unwrap_or_else(|| egui::Rect::from_center_size(screen.center(), egui::Vec2::ZERO))
            .expand(6.);
        let accent = ui.visuals().warn_fg_color;

        // dim everything but the target and swallow clicks underneath
        egui::Area::new(egui::Id::new("tour_overlay"))
            .order(egui::Order::Foreground)
            .fixed_pos(screen.min)
            .show(&ctx, |ui| {
                ui.allocate_rect(screen, egui::Sense::click());
                let dim = egui::Color32::from_black_alpha(150);
                let p = ui.painter();
                for r in [
                    egui::Rect::from_min_max(screen.min, egui::pos2(screen.max.x, target.min.y)),
                    egui::Rect::from_min_max(egui::pos2(screen.min.x, target.max.y), screen.max),
                    egui::Rect::from_min_max(
                        egui::pos2(screen.min.x, target.min.y),
                        egui::pos2(target.min.x, target.max.y),
                    ),
                    egui::Rect::from_min_max(
                        egui::pos2(target.max.x, target.min.y),
                        egui::pos2(screen.max.x, target.max.y),
                    ),
                ] {
                    p.rect_filled(r, 0., dim);
                }
                p.rect_stroke(target, 6., egui::Stroke::new(2., accent), egui::StrokeKind::Outside);
            });

        // the card goes below the target when there is room, otherwise above; egui keeps it on screen
        let card_w = 340.;
        let card_h_guess = 170.;
        let x = if lang::rtl() {
            (target.max.x - card_w).max(screen.min.x + 8.)
        } else {
            target.min.x.min(screen.max.x - card_w - 8.)
        };
        let y = if target.max.y + 12. + card_h_guess < screen.max.y {
            target.max.y + 12.
        } else {
            (target.min.y - 12. - card_h_guess).max(screen.min.y + 8.)
        };

        let total = TOUR_STEPS.len();
        let last = step + 1 == total;
        let mut next = None;

        // keyboard: enter or the "forward" arrow advances, the other arrow goes back (esc is handled in ui())
        let (forward, backward) = if lang::rtl() {
            (egui::Key::ArrowLeft, egui::Key::ArrowRight)
        } else {
            (egui::Key::ArrowRight, egui::Key::ArrowLeft)
        };
        ctx.input_mut(|i| {
            if i.consume_key(egui::Modifiers::NONE, egui::Key::Enter)
                || i.consume_key(egui::Modifiers::NONE, forward)
            {
                next = Some(if last { None } else { Some(step + 1) });
            } else if step > 0 && i.consume_key(egui::Modifiers::NONE, backward) {
                next = Some(Some(step - 1));
            }
        });

        egui::Area::new(egui::Id::new("tour_card"))
            .order(egui::Order::Tooltip)
            .fixed_pos(egui::pos2(x, y))
            .show(&ctx, |ui| {
                egui::Frame::window(ui.style())
                    .stroke(egui::Stroke::new(1., accent))
                    .show(ui, |ui| {
                        ui.set_width(card_w);
                        column(ui, |ui| {
                            row(ui, |ui| {
                                ui.heading(t(title));
                                ui.with_layout(opposite_layout(), |ui| {
                                    ui.weak(format!("{} / {}", step + 1, total));
                                });
                            });
                            ui.add_space(4.);
                            ui.label(t(body));
                            ui.add_space(8.);
                            // next/finish on the far side, back next to it, skip on the reading side
                            ui.with_layout(opposite_layout(), |ui| {
                                if ui
                                    .button(if last { t("tour.finish") } else { t("tour.next") })
                                    .clicked()
                                {
                                    next = Some(if last { None } else { Some(step + 1) });
                                }
                                if step > 0 && ui.button(t("tour.back")).clicked() {
                                    next = Some(Some(step - 1));
                                }
                                ui.with_layout(
                                    if lang::rtl() {
                                        egui::Layout::right_to_left(egui::Align::Center)
                                    } else {
                                        egui::Layout::left_to_right(egui::Align::Center)
                                    },
                                    |ui| {
                                        if ui.small_button(t("tour.skip")).clicked() {
                                            next = Some(None);
                                        }
                                    },
                                );
                            });
                        });
                    });
            });

        if let Some(next) = next {
            self.tour = next;
            if next.is_none() {
                self.config.toured = true;
            }
        }
    }
}
