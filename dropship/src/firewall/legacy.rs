#[cfg(target_os = "windows")]
use crate::firewall;

const PREVIOUS_DROPSHIP_GROUP_NAMES: &[&str] = &[
    "stormy/dropship",    // v2
    "stormy.gg/dropship", // v1
];

const MINA_DEFAULT_GROUPING_NAME: &str = "_MINA Overwatch 2-Server-Selector";

/*TODO
    [ ] fork windows_firewall and have an iter filter closure so we don't have to allocate memory for all rules.
        i did this in dropship v2, that crate does not have this feature
*/

/// is there existing OUTGOING rules from a previous dropship version?
/// matches by group name only. rules may be disabled. rules may not have an application
#[cfg(target_os = "windows")]
pub fn get_legacy_dropship_rules()
-> windows::core::Result<Vec<windows::Win32::NetworkManagement::WindowsFirewall::INetFwRule>> {
    let iter = firewall::win::iter_rules()?
        .with_direction(firewall::win::Direction::Outgoing)
        .filter_map(|r| {
            let g = unsafe { r.Grouping().ok()?.to_string() };

            if PREVIOUS_DROPSHIP_GROUP_NAMES.contains(&g.as_str()) {
                Some(r)
            } else {
                None
            }
        });

    Ok(iter.collect())
}

/// is there existing INCOMING or OUTGOING firewall rules detected from mina?
/// matches by group name only. rules may be disabled. rules may not have an application
#[cfg(target_os = "windows")]
pub fn get_mina_rules()
-> windows::core::Result<Vec<windows::Win32::NetworkManagement::WindowsFirewall::INetFwRule>> {
    let iter = firewall::win::iter_rules()?
        .with_direction(firewall::win::Direction::Outgoing)
        .with_group(MINA_DEFAULT_GROUPING_NAME);

    Ok(iter.collect())
}

#[cfg(target_os = "windows")]
pub fn delete_legacy_rules() -> windows::core::Result<()> {
    // let matched = [get_legacy_dropship_rules()?, get_mina_rules()?]
    //     .into_iter()
    //     .flatten()
    //     .collect::<Vec<_>>();

    let mut matched = get_legacy_dropship_rules()?;
    matched.extend(get_mina_rules()?);

    // let enabled = matched.iter().filter(|r| *r.enabled()).collect::<Vec<_>>();
    // let disabled = matched.iter().filter(|r| !*r.enabled()).collect::<Vec<_>>();

    // if matched.len() > 0 {
    //     log::warn!("{}", {
    //         let mut t = format!(
    //             "<detected conflicting firewall entries> ({})\n",
    //             matched.len(),
    //         );

    //         t += &format!("  <enabled> ({})\n", enabled.len());
    //         for r in enabled.iter() {
    //             t += &format!(
    //                 "    .group {{ {} }}\n    .name {{ {} }}\n\n",
    //                 r.grouping().clone().unwrap_or_default(),
    //                 r.name()
    //             );
    //         }

    //         t += &format!("  <disabled> ({})\n", disabled.len());
    //         for r in disabled {
    //             t += &format!(
    //                 "    .group {{ {} }}\n    .name {{ {} }}\n\n",
    //                 r.grouping().clone().unwrap_or_default(),
    //                 r.name()
    //             );
    //         }

    //         t
    //     });

    //     log::warn!(
    //         "<deleting conflicting firewall entries> ({})",
    //         matched.len()
    //     );
    //     for r in matched.into_iter() {
    //         // {
    //         //     let t = format!("{:#?}", r).to_ascii_lowercase();
    //         //     log::debug!("{}", t);
    //         // }

    //         r.remove()?;
    //     }
    // }

    // if matched.len() > 0 {
    //     log::warn!("{}", {
    //         let mut t = format!(
    //             "<detected conflicting firewall entries> ({})\n",
    //             matched.len(),
    //         );
    //         for r in matched.iter() {
    //             unsafe {
    //                 t += &format!(
    //                     "    .group {{ {} }}\n    .name {{ {} }}\n\n",
    //                     r.Grouping().ok().clone().unwrap_or_default(),
    //                     r.Name().un
    //                 );
    //             }
    //         }
    //     });
    // }

    if matched.len() > 0 {
        log::warn!(
            "<deleting conflicting firewall entries> ({})",
            matched.len()
        );
        for r in matched.into_iter() {
            // {
            //     let t = format!("{:#?}", r).to_ascii_lowercase();
            //     log::debug!("{}", t);
            // }

            if let Err(e) = firewall::win::delete_rule(r) {
                log::error!("{}", e);
            }
        }
    }

    Ok(())
}
