use eframe::{
    self,
    egui::{
        Color32, FontData, FontDefinitions, FontFamily, Margin, Spacing, Stroke, Style, Visuals,
        style::{self, HandleShape},
        vec2,
    },
    epaint::{self},
};

// const BATTLENET_BLUE: Color32 = Color32::from_hex("#148eff").unwrap();
pub const BATTLENET_BLUE: Color32 = Color32::from_rgb(20, 142, 255);

#[allow(dead_code)]
pub const HEX_BDB2FF: Color32 = Color32::from_rgb(189, 178, 255);

#[derive(
    serde::Deserialize, serde::Serialize, Default, Clone, Copy, PartialEq, Eq, strum::AsRefStr,
)]
pub enum Theme {
    #[default]
    Light,
    Dark,
}

pub fn from_theme_alpha(theme: Theme, a: u8) -> Color32 {
    match theme {
        Theme::Light => Color32::from_black_alpha(a),
        Theme::Dark => Color32::from_white_alpha(a),
    }
}

pub fn visuals(style: &mut Style, theme: Theme) {
    style.visuals = Visuals {
        // dark_mode: false,
        // text_options: TextOptions {
        //     alpha_from_coverage: AlphaFromCoverage::LIGHT_MODE_DEFAULT,
        //     max_texture_side: 2048,
        //     font_hinting: true,
        // },
        // override_text_color: None,
        // weak_text_alpha: 0.6,
        // weak_text_color: None,
        // widgets: Widgets {
        //     noninteractive: WidgetVisuals {
        //         weak_bg_fill: Color32::from_gray(248),
        //         bg_fill: Color32::from_gray(248),
        //         bg_stroke: Stroke::new(1.0, Color32::from_gray(190)), // separators, indentation lines
        //         bg_stroke: Stroke::new(1.0, Color32::from_gray(190)), // separators, indentation lines
        //         fg_stroke: Stroke::new(1.0, Color32::from_gray(80)),  // normal text color
        //         corner_radius: CornerRadius::same(2),
        //         expansion: 0.0,
        //     },
        //     inactive: WidgetVisuals {
        //         weak_bg_fill: Color32::from_gray(230), // button background
        //         bg_fill: Color32::from_gray(230),      // checkbox background
        //         bg_stroke: Default::default(),
        //         fg_stroke: Stroke::new(1.0, Color32::from_gray(60)), // button text
        //         corner_radius: CornerRadius::same(2),
        //         expansion: 0.0,
        //     },
        //     hovered: WidgetVisuals {
        //         weak_bg_fill: Color32::from_gray(220),
        //         bg_fill: Color32::from_gray(220),
        //         bg_stroke: Stroke::new(1.0, Color32::from_gray(105)), // e.g. hover over window edge or button
        //         fg_stroke: Stroke::new(1.5, Color32::BLACK),
        //         corner_radius: CornerRadius::same(3),
        //         expansion: 0.0,
        //     },
        //     active: WidgetVisuals {
        //         weak_bg_fill: Color32::from_gray(165),
        //         bg_fill: Color32::from_gray(165),
        //         bg_stroke: Stroke::new(1.0, Color32::BLACK),
        //         fg_stroke: Stroke::new(2.0, Color32::BLACK),
        //         corner_radius: CornerRadius::same(2),
        //         expansion: 0.0,
        //     },
        //     open: WidgetVisuals {
        //         weak_bg_fill: Color32::from_gray(220),
        //         bg_fill: Color32::from_gray(220),
        //         bg_stroke: Stroke::new(1.0, Color32::from_gray(160)),
        //         fg_stroke: Stroke::new(1.0, Color32::BLACK),
        //         corner_radius: CornerRadius::same(2),
        //         expansion: 0.0,
        //     },
        //     ..Widgets::light()
        // },
        // selection: Selection {
        //     bg_fill: Color32::from_rgb(144, 209, 255),
        //     stroke: Stroke::new(1.0, Color32::from_rgb(0, 83, 125)),
        // },
        // hyperlink_color: Color32::from_rgb(0, 155, 255),
        // faint_bg_color: Color32::from_additive_luminance(5), // visible, but barely so
        // extreme_bg_color: Color32::from_gray(255),           // e.g. TextEdit background
        // code_bg_color: Color32::from_gray(230),
        // warn_fg_color: Color32::from_rgb(255, 100, 0), // slightly orange red. it's difficult to find a warning color that pops on bright background.
        // error_fg_color: Color32::from_rgb(255, 0, 0),  // red

        // window_corner_radius: CornerRadius::same(6),
        // window_shadow: Shadow {
        //     offset: [10, 20],
        //     blur: 15,
        //     spread: 0,
        //     color: Color32::from_black_alpha(25),
        // },
        // window_fill: Color32::from_gray(248),
        // window_stroke: Stroke::new(1.0, Color32::from_gray(190)),
        // window_highlight_topmost: true,

        // menu_corner_radius: CornerRadius::same(6),

        // panel_fill: Color32::from_gray(248),

        // popup_shadow: Shadow {
        //     offset: [6, 10],
        //     blur: 8,
        //     spread: 0,
        //     color: Color32::from_black_alpha(25),
        // },

        // resize_corner_size: 12.0,

        // text_cursor: TextCursorStyle {
        //     stroke: Stroke::new(2.0, Color32::from_rgb(0, 83, 125)),
        //     ..Default::default()
        // },

        // clip_rect_margin: 3.0, // should be at least half the size of the widest frame stroke + max WidgetVisuals::expansion
        // button_frame: true,
        // collapsing_header_frame: false,
        // indent_has_left_vline: true,

        // striped: false,
        slider_trailing_fill: true,
        handle_shape: HandleShape::Rect { aspect_ratio: 1.0 },

        // interact_cursor: None,

        // image_loading_spinners: true,

        // numeric_color_space: NumericColorSpace::GammaByte,
        // disabled_alpha: 0.5,
        ..{
            match theme {
                Theme::Light => Visuals::light(),
                Theme::Dark => Visuals::dark(),
            }
        }
    };

    // style.visuals.window_fill = Color32::from_rgb(30, 30, 45);
    // style.visuals.window_fill = Color32::from_rgb(30, 30, 45);
    // style.visuals.window_stroke = Stroke::new(1.0, Color32::from_rgb(100, 100, 120));
    // style.visuals.panel_fill = Color32::from_rgb(35, 35, 40);

    // style.visuals.widgets.noninteractive.bg_fill = Color32::WHITE;

    // style.visuals.window_fill = Color32::from_rgb(40, 40, 40);
    // style.visuals.panel_fill = Color32::from_rgb(20, 20, 20);

    style.visuals.widgets.noninteractive.bg_stroke = Stroke::NONE;

    // style.visuals.widgets.
    // style.visuals.override_text_color = Some(Color32::from_hex("#181818").unwrap());

    // style.visuals.override_text_color = Some(Color32::BLACK);

    match theme {
        Theme::Light => {
            style.visuals.weak_text_alpha = 0.4;
            style.visuals.widgets.noninteractive.fg_stroke.color =
                Color32::from_hex("#181818").unwrap();
            style.visuals.widgets.inactive.fg_stroke.color = Color32::from_hex("#181818").unwrap();
            style.visuals.widgets.hovered.fg_stroke.color = Color32::from_hex("#181818").unwrap();
            style.visuals.widgets.active.fg_stroke.color = Color32::from_hex("#181818").unwrap();

            style.visuals.widgets.inactive.weak_bg_fill = Color32::from_black_alpha(20);
            style.visuals.widgets.active.weak_bg_fill = Color32::from_black_alpha(60);
            style.visuals.widgets.hovered.weak_bg_fill = Color32::from_black_alpha(40);

            style.visuals.warn_fg_color = Color32::from_rgb(252, 157, 31);
            style.visuals.selection = style::Selection {
                bg_fill: Color32::from_rgba_unmultiplied(252, 157, 31, 90),
                stroke: Stroke {
                    // color: Color32::from_rgb(252, 157, 31),
                    // color: Color32::WHITE,
                    color: style.visuals.text_color(),
                    width: 0.2,
                },
            };
        }
        Theme::Dark => {
            style.visuals.weak_text_alpha = 0.4;
            style.visuals.widgets.noninteractive.fg_stroke.color =
                Color32::from_hex("#f9f9f9").unwrap();
            style.visuals.widgets.inactive.fg_stroke.color = Color32::from_hex("#f9f9f9").unwrap();
            style.visuals.widgets.hovered.fg_stroke.color = Color32::from_hex("#f9f9f9").unwrap();
            style.visuals.widgets.active.fg_stroke.color = Color32::from_hex("#f9f9f9").unwrap();

            style.visuals.widgets.inactive.weak_bg_fill = Color32::from_white_alpha(20);
            style.visuals.widgets.active.weak_bg_fill = Color32::from_white_alpha(60);
            style.visuals.widgets.hovered.weak_bg_fill = Color32::from_white_alpha(40);

            style.visuals.warn_fg_color = Color32::from_rgb(252, 157, 31);
            style.visuals.selection = style::Selection {
                bg_fill: Color32::from_rgba_unmultiplied(252, 157, 31, 90),
                stroke: Stroke {
                    // color: Color32::from_rgb(252, 157, 31),
                    // color: Color32::WHITE,
                    color: style.visuals.text_color(),
                    width: 0.2,
                },
            };
        }
    }

    // testing
    {
        // style.visuals.widgets.inactive.fg_stroke.color = Color32::PURPLE;
        // style.visuals.widgets.hovered.fg_stroke.color = Color32::PURPLE;
        // style.visuals.widgets.active.fg_stroke.color = Color32::PURPLE;
        // style.visuals.override_text_color = Some(Color32::PURPLE);
    }

    style.visuals.widgets.inactive.bg_stroke = Stroke::NONE;
    style.visuals.widgets.active.bg_stroke = Stroke::NONE;
    style.visuals.widgets.hovered.bg_stroke = Stroke::NONE;

    // TODO spacing.item_spacing.x = 8; (8)
    // TODO spacing.item_spacing.y = 8; (3)

    style.visuals.hyperlink_color = style.visuals.warn_fg_color;

    style.interaction.tooltip_delay = 0.;
    // style.interaction.interact_radius = 8.;

    // set everything to monospace
    for font_id in style.text_styles.values_mut() {
        font_id.family = FontFamily::Monospace;
    }

    // style.visuals =
    style.visuals.interact_cursor = Some(eframe::egui::CursorIcon::PointingHand);

    style.spacing = Spacing {
        item_spacing: vec2(8.0, 4.0),
        window_margin: Margin::same(16),
        menu_margin: Margin::same(8),
        button_padding: vec2(8.0, 8.0),
        indent: 18.0, // match checkbox/radio-button with `button_padding.x + icon_width + icon_spacing`
        interact_size: vec2(40.0, 18.0),
        slider_width: 100.0,
        slider_rail_height: 18.0,
        combo_width: 100.0,
        text_edit_width: 280.0,
        extra_text_line_spacing: 0.0,
        icon_width: 14.0,
        icon_width_inner: 8.0,
        icon_spacing: 4.0,
        default_area_size: vec2(600.0, 400.0),
        tooltip_width: 500.0,
        menu_width: 400.0,
        menu_spacing: 4.0,
        combo_height: 200.0,
        scroll: Default::default(),
        indent_ends_with_horizontal_line: false,
    };

    // style.spacing.scroll.floating = false;
    style.spacing.scroll = style::ScrollStyle {
        floating: true,
        floating_width: 8.0,
        bar_width: 8.0, // same as floating_width
        dormant_background_opacity: 0.0,
        active_background_opacity: 0.0,
        interact_background_opacity: 0.0,

        // bar_outer_margin: 16.0,
        // bar_inner_margin: 16.0,
        // foreground_color: true,
        // content_margin: Margin::symmetric(32, 32),
        fade: style::ScrollFadeStyle {
            size: 0.0,
            strength: 0.0,
        },

        dormant_handle_opacity: 0.2,
        active_handle_opacity: 0.4,
        interact_handle_opacity: 0.4,

        ..Default::default()
    };
}

pub fn fonts() -> FontDefinitions {
    let mut fonts = FontDefinitions::empty();

    // fonts.font_data.insert(
    //     "font_fallback_2".to_owned(),
    //     std::sync::Arc::new(FontData::from_static(include_bytes!(
    //         "../assets/fonts/JuliaMono-Medium.ttf"
    //     ))),
    // );

    // fonts.font_data.insert(
    //     "font_fallback".to_owned(),
    //     std::sync::Arc::new(FontData::from_static(include_bytes!(
    //         "../assets/fonts/RelaxedTypingMonoJP/RelaxedTypingMonoJP-Medium.ttf"
    //     ))),
    // );

    fonts.font_data.insert(
        "font_text".to_owned(),
        std::sync::Arc::new(FontData::from_static(include_bytes!(
            // "../assets/fonts/DM_Mono/DMMono-Light.ttf"
            // "../assets/fonts/DM_Mono/DMMono-Regular.ttf"
            "../assets/fonts/DM_Mono/DMMono-Medium.ttf"
        ))),
    );

    // fonts
    //     .families
    //     .entry(FontFamily::Monospace)
    //     .or_default()
    //     .insert(0, "font_fallback_2".to_owned());

    // fonts
    //     .families
    //     .entry(FontFamily::Monospace)
    //     .or_default()
    //     .insert(0, "font_fallback".to_owned());

    fonts
        .families
        .entry(FontFamily::Monospace)
        .or_default()
        .insert(0, "font_text".to_owned());

    fonts
}

// fn color(i: u8) -> Color32 {
//     // const ImU32 color = grayed_out ? color_disabled : (ImU32) ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.4f, 1.0f, 1.0f);
//     // const ImU32 color_secondary = grayed_out ? color_disabled_secondary : (ImU32) ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.3f, 1.0f, 1.0f);
//     // const ImU32 color_secondary_faded = grayed_out ? color_disabled_secondary_faded : (ImU32) ImColor::HSV(1.0f - ((i + 1) / 32.0f), 0.2f, 1.0f, 0.4f * 1.0f);

//     let hue = 1.0 - (i + 1) as f32 / 32.0;
//     Color32::from(epaint::HsvaGamma {
//         h: hue,
//         s: 0.4,
//         v: 1.0,
//         a: 1.0,
//     })
// }

pub fn color_inactive(i: usize) -> Color32 {
    let hue = 1.0 - (i + 1) as f32 / 32.0;
    Color32::from(epaint::HsvaGamma {
        h: hue,
        s: 0.4,
        v: 1.,
        a: 1.,
    })
}

pub fn color_active(i: usize) -> Color32 {
    let hue = 1.0 - (i + 1) as f32 / 32.0;
    Color32::from(epaint::HsvaGamma {
        h: hue,
        s: 0.6,
        v: 1.,
        a: 1.,
    })
}

pub fn color_hovered(i: usize) -> Color32 {
    let hue = 1.0 - (i + 1) as f32 / 32.0;
    Color32::from(epaint::HsvaGamma {
        h: hue,
        s: 0.5,
        v: 1.,
        a: 1.,
    })
}

pub fn color_primary(i: usize) -> Color32 {
    let hue = 1.0 - (i + 1) as f32 / 32.0;
    Color32::from(epaint::HsvaGamma {
        h: hue,
        s: 0.4,
        v: 1.0,
        a: 1.0,
    })
}

#[allow(dead_code)]
pub fn color_secondary(i: usize) -> Color32 {
    let hue = 1.0 - (i + 1) as f32 / 32.0;
    Color32::from(epaint::HsvaGamma {
        h: hue,
        s: 0.3,
        v: 1.0,
        a: 1.0,
    })
}

pub fn color_secondary_faded(i: usize) -> Color32 {
    let hue = 1.0 - (i + 1) as f32 / 32.0;
    Color32::from(epaint::HsvaGamma {
        h: hue,
        s: 0.4,
        v: 1.0,
        a: 0.4,
    })
}
