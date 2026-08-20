use std::{collections::HashSet, fmt, path::PathBuf};

use strum::EnumMessage;

use crate::{api, overwatch::ServerSelection, update};

mod dispatch;

pub use dispatch::process_events;

#[derive(strum::EnumMessage, strum::AsRefStr)] // "got .."
/// NOTE events are received (intentionally) in the draw. no system processing
/// should depend on an event being received. events won't be processed while
/// the application is minimized

pub enum Event {
    //
    #[strum(detailed_message = "found new ips from online")]
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

    #[strum(detailed_message = "an executable was added")]
    AddedExecutable(std::path::PathBuf),

    ForceApplyFirewallRequested,
}

impl fmt::Display for Event {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Self::UpdateAvailable(ref data) => write!(f, "version v{} is available", data.version),
            Self::ProcessOpenStatusChange {
                ref process_name,
                open,
                ..
            } => write!(
                f,
                "process \"{}\" {}",
                process_name,
                if open {
                    "was detected as open. blocking servers is not allowed while the game is open"
                } else {
                    "was not detected. blocking servers is now allowed"
                }
            ),
            // Self::FoundApplicationPaths { ref paths } => {
            Self::FoundApplicationPaths(ref paths) => {
                write!(
                    f,
                    "found executable paths from firewall: {:#?}",
                    paths.iter().map(|x| x.display()).collect::<Vec<_>>()
                )
            }

            _ => write!(
                f,
                "{}",
                self.get_detailed_message().unwrap_or(self.as_ref())
            ),
        }
    }
}
