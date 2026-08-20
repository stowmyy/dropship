// use std::path::PathBuf;

// use eframe::egui;

// enum Background {
//     Color { solid: egui::Color32 },
//     Image { path: PathBuf },
//     WebImage { uri: String },
// }

// struct ThemeVariant {

//     /// light or dark base
//     _base: crate::visuals::Theme,

//     /// background
//     background: Background, // none is default image

//     /// override primary color (links, highlights)
//     primary_color: Option<egui::Color32>,
//     text_color: Option<egui::Color32>,
// }

// pub enum ThemeMode {
//     #[default]
//     Light,
//     Dark,
// }

// pub enum Theme {
//     #[default]
//     Dropship,
//     Custom { id: u64 },
// }

// fn draw_current_theme_background(
//     ctx: &egui::Context,
//     theme: crate::visuals::Theme,
//     config: &DropshipThemeConfig,
// )
// }
