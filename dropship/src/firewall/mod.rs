#[cfg(target_os = "windows")]
use std::path::PathBuf;

pub mod win;

// handles previous versions of dropship
// also handles uninstalling mina
pub mod legacy;

pub mod applications;

pub const DROPSHIP_GROUP_NAME: &str = "stormy/dropship/v3";
// pub const DROPSHIP_RULE_NAME: &str = "dropship/overwatch";
const OVERWATCH_RULE_NAME: &str = "Overwatch Application";
// const OVERWATCH_RULE_DESCRIPTION: &str = "Overwatch Application";

/// gets any dropship rules
/// matches on group name alone
#[cfg(target_os = "windows")]
pub fn get_dropship_rules() -> windows::core::Result<win::RuleIter> {
    let iter = win::iter_rules()?
        .with_group(DROPSHIP_GROUP_NAME)
        .with_direction(win::Direction::Outgoing);

    Ok(iter)
}

#[cfg(target_os = "windows")]
pub fn get_dropship_rule(
    path: &PathBuf,
) -> windows::core::Result<Option<windows::Win32::NetworkManagement::WindowsFirewall::INetFwRule>> {
    let rule = win::iter_rules()?
        .with_group(DROPSHIP_GROUP_NAME)
        .with_direction(win::Direction::Outgoing)
        .find(|x| unsafe { x.ApplicationName().ok() == Some(win::path_to_bstr(&path)) });

    Ok(rule)
}

// #[cfg(target_os = "windows")]
// pub fn delete_dropship_rule(path: &PathBuf) -> windows::core::Result<()> {
//     match get_dropship_rule(path) {
//         Ok(rule) => {
//             if let Some(rule) = rule {
//                 if let Err(e) = unsafe { rule.SetEnabled(false.into()) } {
//                     log::error!("{}", e);
//                 }

//                 let name = windows::core::BSTR::from("delete");
//                 if let Err(e) = unsafe { rule.SetName(&name) } {
//                     log::error!("{}", e);
//                 }

//                 if let Ok(rules) = unsafe { win::get_rules() } {
//                     if let Err(e) = unsafe { rules.Remove(&name) } {
//                         log::error!("{}", e);
//                     }
//                 }
//             }
//         }
//         Err(e) => {
//             log::error!("{}", e);
//         }
//     }

//     Ok(())
// }

#[cfg(target_os = "windows")]
pub fn delete_dropship_rules() -> windows::core::Result<()> {
    let rules = unsafe { win::get_rules()? };

    let dropship_rules = get_dropship_rules()?;

    for r in dropship_rules {
        if let Err(e) = unsafe { r.SetEnabled(false.into()) } {
            log::error!("{}", e);
        }

        let name = windows::core::BSTR::from("delete");
        if let Err(e) = unsafe { r.SetName(&name) } {
            log::error!("{}", e);
        }

        if let Err(e) = unsafe { rules.Remove(&name) } {
            log::error!("{}", e);
        }
    }

    Ok(())
}

// #[cfg(target_os = "windows")]
// pub fn delete_dropship_rules() -> windows::core::Result<()> {
//     unsafe {
//         let rules = win::get_rules()?;
//         rules.Remove(&windows::core::BSTR::from(DROPSHIP_RULE_NAME))?;
//         Ok(())
//     }
// }

/// gets any official overwatch rules
/// matches on INBOUND rule names alone
#[cfg(target_os = "windows")]
pub fn get_blizzard_overwatch_rules() -> windows::core::Result<win::RuleIter> {
    let iter = win::iter_rules()?
        .with_name(OVERWATCH_RULE_NAME)
        .with_direction(win::Direction::Incoming);

    Ok(iter)
}

#[cfg(target_os = "windows")]
pub fn get_firewall_state_but_if_different_then_disable_them_all() -> windows::core::Result<()> {
    let mixed = 'result: {
        let mut prev_state = None;

        unsafe {
            for rule in get_dropship_rules()? {
                let enabled = rule.Enabled()?.as_bool();

                if let Some(prev_state) = prev_state {
                    if enabled != prev_state {
                        break 'result true;
                    }
                } else {
                    prev_state = Some(enabled);
                }
            }
        }
        false
    };

    if mixed {
        log::warn!("mix of enabled and disabled dropship rules detected. setting all to disabled.");

        for x in get_dropship_rules()? {
            unsafe {
                x.SetEnabled(false.into())?;
            }
        }
    }

    Ok(())
}
