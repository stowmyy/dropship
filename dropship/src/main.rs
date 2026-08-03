// probably fine to use this xd
#![feature(thread_id_value)]
//
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")] // hide console window on windows in release

use eframe::egui;
use tokio::sync::mpsc;

mod api;
mod app;
mod assets;
mod components;
mod dropship;
mod firewall;
mod logger;
mod ping;
mod ping_icon;
mod process;
mod update;
mod visuals;

mod overwatch;

pub const APP_WIDTH: f32 = 999f32.min(app::HERO_BG_SIZE.x);
pub const APP_HEIGHT: f32 = 666f32.min(app::HERO_BG_SIZE.y);

#[tokio::main]
async fn main() -> eframe::Result {
    // let mut initialization_errors = vec![];

    // need this so winit does not steal the com mode and make it not multithreaded
    // #[cfg(target_os = "windows")]
    // unsafe {
    //     if let Err(e) = windows::Win32::System::Com::CoInitializeEx(
    //         None,
    //         windows::Win32::System::Com::COINIT_MULTITHREADED,
    //     )
    //     .ok()
    //     {
    //         initialization_errors.push(e);
    //     }
    // }

    // #[cfg(target_os = "windows")]
    // let _com_cleanup = scopeguard::guard((), |()| unsafe {
    //     windows::Win32::System::Com::CoUninitialize()
    // });

    let native_options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([APP_WIDTH, APP_HEIGHT])
            // .with_min_inner_size([width, height])
            // .with_min_inner_size([1., 1.])
            // .with_max_inner_size(app::HERO_BG_SIZE)
            // .with_max_inner_size([width, height])
            .with_active(true)
            .with_maximize_button(false)
            // .with_decorations(false)
            // .with_transparent(true)
            .with_resizable(false)
            .with_drag_and_drop(false) // on windows this initializes com with COINIT_APARTMENTTHREADED
            .with_icon(
                eframe::icon_data::from_png_bytes(&include_bytes!("../assets/white-bolts.png")[..])
                    .expect("failed to load icon"),
            ),

        // hardware_acceleration // TODO maybe multiple targets
        persist_window: false,
        ..Default::default()
    };

    eframe::run_native(
        "dropship",
        native_options,
        Box::new(|cc| {
            egui_extras::install_image_loaders(&cc.egui_ctx);

            let (logs_tx, logs_rx) = mpsc::unbounded_channel::<logger::Message>();
            let logger = logger::Logger {
                max_level: log::LevelFilter::Info,
                ctx: Some(cc.egui_ctx.clone()),
                tx: logs_tx,
            };
            let _ = log::set_boxed_logger(Box::new(logger));
            log::set_max_level(log::LevelFilter::Debug);

            // for e in initialization_errors {
            //     log::error!("{}", e);
            // }

            Ok(Box::new(app::TemplateApp::new(cc, logs_rx)))
        }),
    )
}
