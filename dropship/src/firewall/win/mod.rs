use std::collections::HashSet;
use std::path::PathBuf;

use super::applications;
use crate::api;

mod windows_fw;

pub use windows_fw::{
    Direction, RuleIter, bstr_to_path, delete_rule, flush_dns, get_rules, iter_rules, path_to_bstr,
    reset_global_windows_firewall,
};

pub use windows_wfp::{WfpConnection, delete_dropship_wfp};

pub mod windows_wfp;

/* TODO

    - i could try https://crates.io/crates/windows-wfp
    which would allow filtering at a lower level, and while the app is open
    it also supports persistence?

*/

// blocks @blocked_servers for @path
// if a rule
//     exists: it's modified
//     doesn't exist: it's created
// that rule is now enabled
/* fn block_ips_for_path(
    path: &PathBuf,
    blocked_servers: &HashSet<api::KnownServer>,
) -> windows::core::Result<()> {
    //

    // we want to block these ips and enable the rule
    let desired_remote_addresses = {
        let ips = blocked_servers
            .iter()
            .map(|x| x.block.clone())
            .collect::<Vec<_>>()
            .join(",");

        windows::core::BSTR::from(ips)
    };

    // if there are no blocks, disable the rule because it will default
    // to blocking "*" and that is not good
    let desired_enabled_state = !blocked_servers.is_empty();

    // get the rule, if it was missing create and get the rule
    let rule = {
        if let Some(existing_rule) = firewall::get_dropship_rule(&path)? {
            // log::debug!("existing rule found for {}", &path.display());

            existing_rule
        } else {
            log::debug!("creating rule for \"{}\"", &path.display());

            windows_fw::create_rule(
                firewall::DROPSHIP_RULE_NAME,
                firewall::DROPSHIP_GROUP_NAME,
                &path,
                windows_fw::Direction::Outgoing,
                windows_fw::Action::Block,
                desired_enabled_state,
            )?
        }
    };

    // apply changes. we query the state first and apply if it doesn't already match
    unsafe {
        /* NOTE
            impossible to check differences :c

            [src\firewall\win\mod.rs:80:9] rule.RemoteAddresses()? = 64.224.26.0/255.255.254.0
            [src\firewall\win\mod.rs:81:9] desired_remote_addresses.clone() = 64.224.26.0/23
            [src\firewall\win\mod.rs:82:9] rule.RemoteAddresses()? != desired_remote_addresses.clone() = true
        */

        if rule.RemoteAddresses()? != desired_remote_addresses {
            // log::info!(
            //     "applying new ip blocks for \"{}\". {:?}",
            //     &path.display(),
            //     blocked_servers
            //         .iter()
            //         .map(|s| &s.token)
            //         .collect::<Vec<_>>()
            //         .clone()
            // );

            rule.SetRemoteAddresses(&desired_remote_addresses)?;
        }

        if rule.Enabled()?.as_bool() != desired_enabled_state {
            log::info!(
                "{} dropship for \"{}\"",
                if desired_enabled_state == true {
                    "enabling"
                } else {
                    "disabling"
                },
                &path.display(),
            );

            rule.SetEnabled(desired_enabled_state.into())?;
        }
    }

    Ok(())
}

/// queries possible paths. then blocks servers.
/// @param already_known_paths is a hint, more paths than provided could be affected
/// a rule per path will exist and be enabled after this completes
///
/// returns the affected paths
// pub fn apply_blocked_ips(
//     blocked_servers: &HashSet<api::KnownServer>,

//     already_known_paths: &HashSet<PathBuf>,
// ) -> windows::core::Result<HashSet<PathBuf>> {
//     //
//     // create_missing_dropship_rules(&already_known_paths)?;

//     let paths = applications::get_known_valid_application_paths(&already_known_paths)?;

//     for path in &paths {
//         match block_ips_for_path(&path, &blocked_servers) {
//             Ok(_) => {
//                 // log::debug!("updated blocks for path \"{}\".", &path.display());
//             }
//             Err(e) => {
//                 log::error!(
//                     "failed to block ips for path \"{}\". ({})",
//                     &path.display(),
//                     e.to_string()
//                 );
//             }
//         }
//     }

//     log::info!(
//         "configured pc to block {}",
//         if blocked_servers.is_empty() {
//             "nothing".to_string()
//         } else {
//             format!(
//                 "{:?}",
//                 blocked_servers
//                     .iter()
//                     .map(|s| &s.token)
//                     .collect::<Vec<_>>()
//                     .clone()
//             )
//         }
//     );

//     Ok(paths)
// }

*/

pub fn apply_blocked_ips_wfp(
    connection: &mut WfpConnection,

    blocked_servers: &HashSet<api::KnownServer>,

    already_known_paths: &HashSet<PathBuf>,
) -> windows::core::Result<HashSet<PathBuf>> {
    let paths = applications::get_known_valid_application_paths(&already_known_paths)?;

    // let mut connection = WfpConnection::new(true)?;

    connection.wipe_all_and_block_servers_for_applications(blocked_servers, &paths)?;

    Ok(paths)
}
