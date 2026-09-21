use std::{collections::HashSet, fmt, path::PathBuf};

use crate::lang::{t, tf};
use crate::{api, overwatch::ServerSelection, update};

mod dispatch;

pub use dispatch::process_events;

/// Events are (intentionally) processed in the draw pass, so sending one must also
/// request a repaint. Otherwise results such as `ApiResponse` or `ProcessOpenStatusChange`
/// wait until the user focuses the window again.
#[derive(Clone)]
pub struct EventSender {
    tx: tokio::sync::mpsc::UnboundedSender<Event>,
    ctx: Option<eframe::egui::Context>,
}

impl EventSender {
    pub fn new(
        tx: tokio::sync::mpsc::UnboundedSender<Event>,
        ctx: Option<eframe::egui::Context>,
    ) -> Self {
        Self { tx, ctx }
    }

    pub fn send(&self, event: Event) -> Result<(), tokio::sync::mpsc::error::SendError<Event>> {
        let result = self.tx.send(event);
        if let Some(ctx) = &self.ctx {
            ctx.request_repaint();
        }
        result
    }
}

#[derive(strum::AsRefStr)] // "got .."
/// NOTE events are received (intentionally) in the draw. no system processing
/// should depend on an event being received. events won't be processed while
/// the application is minimized

pub enum Event {
    //
    ApiResponse(api::DropshipApiData),

    // #[strum(detailed_message  = "")]
    // PlayerUninstall {},
    // #[strum(detailed_message = "an update is available")]
    UpdateAvailable(update::AvailableUpdate),

    // pong
    Pong {
        ip: String,
        pong: Result<f32, String>,
    },

    // #[strum(detailed_message = "found new ips from online")]
    ApplicationUpdateStatusChange(update::UpdatingStatus),

    ProcessOpenStatusChange {
        process_name: String,
        open: bool,
        path: Option<PathBuf>,
    },

    FoundApplicationPaths(HashSet<PathBuf>),
    FirewallConfigApplied {
        blocked_servers: ServerSelection,
    },

    /// called when we want to visually communicate a loading state change.
    /// probably only going to use this for firewall changes
    DropshipLoadingStateChange(bool),

    AddedExecutable(std::path::PathBuf),

    ForceApplyFirewallRequested,
}

impl fmt::Display for Event {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Self::UpdateAvailable(ref data) => {
                write!(f, "{}", tf("evt.version_available", &[&data.version]))
            }
            Self::ProcessOpenStatusChange {
                ref process_name,
                open,
                ..
            } => write!(
                f,
                "{}",
                tf(
                    if open { "evt.process_open" } else { "evt.process_closed" },
                    &[process_name]
                )
            ),
            // Self::FoundApplicationPaths { ref paths } => {
            Self::FoundApplicationPaths(ref paths) => {
                write!(
                    f,
                    "{}",
                    tf(
                        "evt.found_paths",
                        &[&format!("{:#?}", paths.iter().map(|x| x.display()).collect::<Vec<_>>())]
                    )
                )
            }
            Self::ApiResponse(..) => write!(f, "{}", t("evt.api_response")),
            Self::AddedExecutable(..) => write!(f, "{}", t("evt.executable_added")),

            _ => write!(f, "{}", self.as_ref()),
        }
    }
}
