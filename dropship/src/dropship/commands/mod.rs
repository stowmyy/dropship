use std::{
    collections::HashSet,
    fmt,
    path::PathBuf,
    sync::{Arc, atomic},
};

use crate::api;
use crate::lang::{t, tf};

mod dispatch;

pub use dispatch::start_processing_commands;

#[derive(strum::AsRefStr)] // "failed .."
/// NOTE commands are received in a background task and processed asynchronously
/// events are sent to update the ui and persistence. events are not processed when
/// the app is minimized because events are (intentionally) processed in the draw
pub enum Command {
    /// web attempt api data sync
    UpdateConfigFromRemote,

    /// web attempt version check
    VersionCheck,

    // update the machine's firewall based on player config
    ApplyFirewallConfig {
        blocked_servers: HashSet<api::KnownServer>,
        already_known_paths: HashSet<PathBuf>,
    },

    // ping
    Ping {
        ip: String,
    },

    // if a player hates dropship and wants to leave no trace
    // #[strum(detailed_message = "")]
    // TODO
    // Uninstall,

    //
    ApplicationUpdate {
        binary_download: String,
        download_total_size: Arc<atomic::AtomicU64>,
        downloaded_size: Arc<atomic::AtomicU64>,
    },

    ProcessCheck {
        process_name: String,
    },

    // #[strum(detailed_message = "adding executable path to dropship")]
    AddExecutable {
        path: std::path::PathBuf,
    },

    ForceApplyFirewallRequested,
}

impl fmt::Display for Command {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Self::AddExecutable { ref path } => {
                write!(f, "{}", tf("cmd.add_executable", &[&path.display()]))
            }
            Self::UpdateConfigFromRemote => write!(f, "{}", t("cmd.update_config")),
            Self::VersionCheck => write!(f, "{}", t("cmd.version_check")),
            Self::ApplyFirewallConfig { .. } => write!(f, "{}", t("cmd.apply_firewall")),
            Self::ApplicationUpdate { .. } => write!(f, "{}", t("cmd.app_update")),
            Self::ProcessCheck { .. } => write!(f, "{}", t("cmd.process_check")),
            _ => write!(f, "{}", self.as_ref()),
        }
    }
}
