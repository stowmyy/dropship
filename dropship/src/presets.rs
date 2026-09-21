//! blocking presets: a named set of servers to block, applied with one click or a hotkey.
//! stored by server token (not bit) so a preset survives the api renumbering servers.

use crate::{
    app::{TemplateApp, row, start_align},
    lang::{t, tf},
    overwatch::ServerSelection,
};
use eframe::egui;

#[derive(Clone, serde::Serialize, serde::Deserialize)]
pub struct Preset {
    pub name: String,
    pub key: Option<egui::Key>,
    pub blocked: Vec<String>,
}

/// the preset a fresh install starts with: "EU" blocks the middle east server only, which is
/// what most players in the region use dropship for
pub fn default_presets() -> Vec<Preset> {
    vec![Preset { name: "EU".into(), key: Some(egui::Key::F1), blocked: vec!["gmec2".into()] }]
}

/// state of the create/edit dialog (not persisted)
pub struct PresetEditor {
    /// `None` for a new preset, else the index being edited
    pub idx: Option<usize>,
    pub step: u8,
    pub name: String,
    pub key: Option<egui::Key>,
    /// waiting for a key press to bind
    pub listening: bool,
    pub blocked: Vec<String>,
}

impl TemplateApp {
    /// the preset's servers as a selection over the current list
    pub(crate) fn preset_selection(&self, p: &Preset) -> ServerSelection {
        let mut sel = ServerSelection::none();
        for s in self.known_servers() {
            if p.blocked.iter().any(|tk| tk == &s.token) {
                sel.set(s);
            }
        }
        sel
    }

    /// applies preset `i`: blocks its servers and allows every other one (same rules as the list)
    pub(crate) fn apply_preset(&mut self, i: usize) {
        let Some(p) = self.config.presets.get(i).cloned() else { return };
        let sel = self.preset_selection(&p);
        let servers = self.known_servers();
        if !servers.is_empty() && servers.iter().all(|s| sel.has(s)) {
            log::warn!("{}", tf("msg.preset_all_blocked", &[&p.name]));
            return;
        }
        if sel.bits() != self.config.desired_blocked_servers.bits() {
            self.config.desired_blocked_servers = sel;
            self.apply_blocked_servers_to_firewall();
        }
        log::info!("{}", tf("msg.preset_applied", &[&p.name]));
    }

    /// a preset's hotkey was pressed this frame? (checked from the ui, outside text fields)
    pub(crate) fn pressed_preset(&self, ctx: &egui::Context) -> Option<usize> {
        ctx.input(|i| self.config.presets.iter().position(|p| p.key.is_some_and(|k| i.key_pressed(k))))
    }

    pub(crate) fn open_preset_editor(&mut self, idx: Option<usize>) {
        let p = idx.and_then(|i| self.config.presets.get(i)).cloned();
        self.preset_editor = Some(PresetEditor {
            idx,
            step: 0,
            name: p.as_ref().map_or(String::new(), |p| p.name.clone()),
            key: p.as_ref().and_then(|p| p.key),
            listening: false,
            blocked: p.map_or(Vec::new(), |p| p.blocked),
        });
    }

    /// the row of preset buttons: left click applies, right click edits, `+` creates.
    /// returns the row's rect for the tour
    pub(crate) fn presets_row(&mut self, ui: &mut egui::Ui) -> egui::Rect {
        let mut fire = None;
        let mut edit = None;
        let r = row(ui, |ui| {
            ui.spacing_mut().item_spacing.x = 4.;
            ui.label(egui::RichText::new(t("presets.label")).small());
            for (i, p) in self.config.presets.iter().enumerate() {
                let active = self.preset_selection(p).bits() == self.config.desired_blocked_servers.bits();
                let text = match p.key {
                    Some(k) => format!("{} · {}", p.name, k.name()),
                    None => p.name.clone(),
                };
                let b = ui.selectable_label(active, egui::RichText::new(text).small());
                let hint = tf(
                    "presets.hint",
                    &[&if p.blocked.is_empty() { t("presets.nothing").to_owned() } else { p.blocked.join(", ") }],
                );
                let b = b.on_hover_text_at_pointer(hint);
                if b.clicked() {
                    fire = Some(i);
                } else if b.secondary_clicked() {
                    edit = Some(Some(i));
                }
            }
            if ui.small_button("+").on_hover_text_at_pointer(t("presets.new")).clicked() {
                edit = Some(None);
            }
        });
        if let Some(i) = fire {
            self.apply_preset(i);
        }
        if let Some(e) = edit {
            self.open_preset_editor(e);
        }
        r.response.rect
    }

    /// the create/edit dialog, two steps: name + key, then the servers
    pub(crate) fn preset_editor_ui(&mut self, ui: &mut egui::Ui) {
        let Some(mut ed) = self.preset_editor.take() else { return };
        let servers = self.known_servers().to_vec();
        let was_listening = ed.listening;
        let (mut close, mut save, mut delete) = (false, false, false);

        let modal = egui::Modal::new(egui::Id::new("preset_editor")).show(ui.ctx(), |ui| {
            ui.set_max_width(340.);
            ui.set_max_height(440.);
            ui.with_layout(egui::Layout::top_down(start_align()), |ui| {
                ui.heading(if ed.idx.is_some() { t("presets.title_edit") } else { t("presets.title_new") });
                ui.add_space(8.);
                if ed.step == 0 {
                    ui.label(t("presets.step_name"));
                    let te = ui.add(
                        egui::TextEdit::singleline(&mut ed.name)
                            .hint_text(t("presets.name_hint"))
                            .horizontal_align(start_align())
                            .desired_width(f32::INFINITY),
                    );
                    if ed.idx.is_none() && ed.name.is_empty() && !ed.listening {
                        te.request_focus();
                    }
                    ui.add_space(10.);
                    ui.label(t("presets.step_key"));
                    row(ui, |ui| {
                        let label = if ed.listening {
                            t("presets.listening").to_owned()
                        } else {
                            ed.key.map_or(t("presets.no_key").to_owned(), |k| k.name().to_owned())
                        };
                        let b = ui.add(egui::Button::new(label).selected(ed.listening).min_size(egui::vec2(170., 24.)));
                        if b.on_hover_text_at_pointer(t("presets.key_hint")).clicked() {
                            ed.listening = !ed.listening;
                        }
                        if ed.key.is_some() && !ed.listening && ui.small_button(t("presets.clear")).clicked() {
                            ed.key = None;
                        }
                    });
                    if ed.listening {
                        let pressed = ui.input(|i| {
                            i.events.iter().find_map(|e| match e {
                                egui::Event::Key { key, pressed: true, .. } => Some(*key),
                                _ => None,
                            })
                        });
                        if let Some(k) = pressed {
                            if k != egui::Key::Escape {
                                ed.key = Some(k);
                            }
                            ed.listening = false;
                        }
                    }
                    ui.add_space(14.);
                    row(ui, |ui| {
                        if ui.add_enabled(!ed.name.trim().is_empty(), egui::Button::new(t("presets.next"))).clicked() {
                            ed.step = 1;
                        }
                        if ui.button(t("presets.cancel")).clicked() {
                            close = true;
                        }
                    });
                } else {
                    ui.label(t("presets.step_servers"));
                    row(ui, |ui| {
                        if ui.small_button(t("presets.all")).clicked() {
                            ed.blocked = servers.iter().map(|s| s.token.clone()).collect();
                        }
                        if ui.small_button(t("presets.invert")).clicked() {
                            ed.blocked = servers.iter().filter(|s| !ed.blocked.contains(&s.token)).map(|s| s.token.clone()).collect();
                        }
                        if ui.small_button(t("presets.none")).clicked() {
                            ed.blocked.clear();
                        }
                    });
                    ui.add_space(4.);
                    egui::ScrollArea::vertical().max_height(300.).show(ui, |ui| {
                        for s in &servers {
                            let mut on = ed.blocked.contains(&s.token);
                            row(ui, |ui| {
                                if ui.checkbox(&mut on, "").changed() {
                                    if on {
                                        ed.blocked.push(s.token.clone());
                                    } else {
                                        ed.blocked.retain(|tk| tk != &s.token);
                                    }
                                }
                                let name = egui::RichText::new(&s.title);
                                ui.label(if on { name.color(ui.visuals().error_fg_color) } else { name });
                                ui.label(egui::RichText::new(s.token.to_ascii_uppercase()).weak().small());
                            });
                        }
                    });
                    let n_blocked = servers.iter().filter(|s| ed.blocked.contains(&s.token)).count();
                    let all = !servers.is_empty() && n_blocked == servers.len();
                    ui.add_space(6.);
                    if all {
                        ui.colored_label(ui.visuals().error_fg_color, t("presets.cannot_all"));
                    } else {
                        ui.label(egui::RichText::new(tf("presets.count", &[&n_blocked, &servers.len()])).weak());
                    }
                    ui.add_space(10.);
                    row(ui, |ui| {
                        if ui.add_enabled(!all, egui::Button::new(t("presets.save"))).clicked() {
                            save = true;
                        }
                        if ui.button(t("presets.back")).clicked() {
                            ed.step = 0;
                        }
                        if ed.idx.is_some() && ui.button(egui::RichText::new(t("presets.delete")).color(ui.visuals().error_fg_color)).clicked() {
                            delete = true;
                        }
                    });
                }
            });
        });
        // esc while waiting for a key only cancels the wait
        if !was_listening && modal.should_close() {
            close = true;
        }

        if save {
            let name = ed.name.trim().to_owned();
            let p = Preset { name: name.clone(), key: ed.key, blocked: ed.blocked.clone() };
            match ed.idx {
                Some(i) if i < self.config.presets.len() => self.config.presets[i] = p,
                _ => self.config.presets.push(p),
            }
            log::info!("{}", tf("msg.preset_saved", &[&name]));
        } else if delete {
            if let Some(i) = ed.idx {
                if i < self.config.presets.len() {
                    let p = self.config.presets.remove(i);
                    log::info!("{}", tf("msg.preset_deleted", &[&p.name]));
                }
            }
        } else if !close {
            self.preset_editor = Some(ed);
        }
    }
}
