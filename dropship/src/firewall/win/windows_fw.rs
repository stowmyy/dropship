use std::{
    ffi::OsString,
    os::windows::ffi::{OsStrExt, OsStringExt},
    path::PathBuf,
};

use windows::{
    Win32::{
        NetworkManagement::WindowsFirewall::{
            INetFwPolicy2, INetFwRule, INetFwRules, NET_FW_IP_PROTOCOL_ANY, NET_FW_PROFILE2_ALL,
            NetFwPolicy2, NetFwRule,
        },
        System::{
            Com::{CLSCTX_ALL, CoCreateInstance},
            Ole::IEnumVARIANT,
            Variant::{VARIANT, VariantClear},
        },
    },
    core::{BSTR, Interface},
};

pub fn path_to_bstr(path: &PathBuf) -> BSTR {
    let wide: Vec<u16> = path.as_os_str().encode_wide().collect();
    BSTR::from_wide(&wide)
}

pub fn bstr_to_path(bstr: &BSTR) -> PathBuf {
    PathBuf::from(OsString::from_wide(bstr))
}

pub fn create_rule(
    name: &str,
    group: &str,
    path: &PathBuf,
    direction: Direction,
    action: Action,
    enabled: bool,
) -> windows::core::Result<INetFwRule> {
    debug_assert!(path_to_bstr(path) == path_to_bstr(path));

    unsafe {
        let rules = get_rules()?;
        let rule: INetFwRule = CoCreateInstance(&NetFwRule, None, CLSCTX_ALL)?;

        rule.SetName(&BSTR::from(name))?;
        rule.SetGrouping(&BSTR::from(group))?;
        rule.SetApplicationName(&path_to_bstr(path))?;
        rule.SetAction(action.into())?;
        rule.SetEnabled(enabled.into())?;
        rule.SetDirection(direction.into())?;
        rule.SetProfiles(NET_FW_PROFILE2_ALL.0)?;
        rule.SetProtocol(NET_FW_IP_PROTOCOL_ANY.0)?;

        rules.Add(&rule)?;
        Ok(rule)
    }
}

/// NOTE deletes by name, can remove multiple with the same name
pub fn delete_rule(rule: INetFwRule) -> windows::core::Result<()> {
    unsafe {
        let name = rule.Name()?;

        let rules = get_rules()?;
        rules.Remove(&name)?;
        Ok(())
    }
}

pub unsafe fn get_rules() -> windows::core::Result<INetFwRules> {
    unsafe {
        let policy: INetFwPolicy2 = CoCreateInstance(&NetFwPolicy2, None, CLSCTX_ALL)?;
        policy.Rules()
    }
}

pub fn iter_rules() -> windows::core::Result<RuleIter> {
    unsafe {
        let _rules = get_rules()?;
        let enum_variant = _rules._NewEnum()?.cast::<IEnumVARIANT>()?;

        Ok(RuleIter {
            _rules,
            enum_variant,
            group: None,
            direction: None,
            name: None,
        })
    }
}

pub struct RuleIter {
    _rules: INetFwRules,
    enum_variant: IEnumVARIANT,
    group: Option<BSTR>,
    direction: Option<Direction>,
    name: Option<BSTR>,
}

impl RuleIter {
    pub fn with_group(mut self, group: &str) -> Self {
        self.group = Some(BSTR::from(group));
        self
    }

    pub fn with_direction(mut self, direction: Direction) -> Self {
        self.direction = Some(direction);
        self
    }

    pub fn with_name(mut self, name: &str) -> Self {
        self.name = Some(BSTR::from(name));
        self
    }
}

impl Iterator for RuleIter {
    // type Item = windows::core::Result<INetFwRule>;
    type Item = INetFwRule;

    fn next(&mut self) -> Option<Self::Item> {
        unsafe {
            loop {
                let mut variants = [VARIANT::default(); 1];
                let mut fetched = 0;

                let hr = self.enum_variant.Next(&mut variants, &mut fetched);

                let variant = std::mem::take(&mut variants[0]);
                let _guard = scopeguard::guard(variant, |mut v| {
                    let _ = VariantClear(&mut v);
                });

                match hr.ok() {
                    Ok(()) if fetched == 1 => {}
                    Ok(()) => return None,
                    Err(e) => {
                        log::error!("{}", e);

                        return None;
                    }
                }

                let dispatch = _guard.Anonymous.Anonymous.Anonymous.pdispVal.as_ref()?;
                let rule = dispatch.cast::<INetFwRule>().ok()?;

                if let Some(ref target) = self.group {
                    if rule.Grouping().ok()? != *target {
                        continue;
                    }
                }

                if let Some(ref target) = self.direction {
                    if rule.Direction().ok()? != windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_RULE_DIRECTION::from(*target) {
                        continue;
                    }
                }

                if let Some(ref target) = self.name {
                    if rule.Name().ok()? != *target {
                        continue;
                    }
                }

                return Some(rule);
            }
        }
    }
}

// web https://github.com/lhenry-dev/windows-firewall-rs/blob/main/src/firewall_rule/types/direction.rs
#[derive(Copy, Clone)]
pub enum Direction {
    Incoming,
    Outgoing,
    // Max,
}

impl From<Direction> for windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_RULE_DIRECTION {
    fn from(direction: Direction) -> Self {
        match direction {
            Direction::Incoming => {
                windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_RULE_DIR_IN
            }
            Direction::Outgoing => {
                windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_RULE_DIR_OUT
            } // Direction::Max => {
              //     windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_RULE_DIR_MAX
              // }
        }
    }
}

// web https://github.com/lhenry-dev/windows-firewall-rs/blob/main/src/firewall_rule/types/action.rs
#[derive(Copy, Clone)]
pub enum Action {
    Block,
    Allow,
    // Max,
}

impl From<Action> for windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_ACTION {
    fn from(action: Action) -> Self {
        match action {
            Action::Block => {
                windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_ACTION_BLOCK
            }
            Action::Allow => {
                windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_ACTION_ALLOW
            } // Action::Max => windows::Win32::NetworkManagement::WindowsFirewall::NET_FW_ACTION_MAX,
        }
    }
}
