use eframe::egui;

pub const ICON_SIGNAL_BARS: egui::ImageSource =
    egui::include_image!("../assets/icons/icon-signal-bars.svg");
pub const ICON_SIGNAL_BARS_GOOD: egui::ImageSource =
    // egui::include_image!("../assets/icons/icon-signal-bars-good.svg");
    egui::include_image!("../assets/icons/icon-signal-bars-good-duotone.svg");
pub const ICON_SIGNAL_BARS_FAIR: egui::ImageSource =
    // egui::include_image!("../assets/icons/icon-signal-bars-fair.svg");
    egui::include_image!("../assets/icons/icon-signal-bars-fair-duotone.svg");
pub const ICON_SIGNAL_BARS_WEAK: egui::ImageSource =
    // egui::include_image!("../assets/icons/icon-signal-bars-weak.svg");
    egui::include_image!("../assets/icons/icon-signal-bars-weak-duotone.svg");
pub const ICON_SIGNAL_BARS_SLASH: egui::ImageSource =
    // egui::include_image!("../assets/icons/icon-signal-bars-slash.svg");
    egui::include_image!("../assets/icons/icon-signal-bars-slash-duotone.svg");

pub fn ping_icon(ping: f32) -> egui::ImageSource<'static> {
    match ping {
        ping if ping <= 30.0 => ICON_SIGNAL_BARS,
        30.0..=60.0 => ICON_SIGNAL_BARS_GOOD,
        60.0..=90.0 => ICON_SIGNAL_BARS_FAIR,
        ping if ping > 90.0 => ICON_SIGNAL_BARS_WEAK,
        _ => ICON_SIGNAL_BARS_SLASH, // nan space
    }
}

pub fn ping_icon_cycle(time: f64) -> egui::ImageSource<'static> {
    let i = (((time % 2.) / 2.) * 4.) as usize;

    match i {
        0 => ICON_SIGNAL_BARS_WEAK,
        1 => ICON_SIGNAL_BARS_FAIR,
        2 => ICON_SIGNAL_BARS_GOOD,
        3 | _ => ICON_SIGNAL_BARS,
    }
}
