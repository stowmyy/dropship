//! localization. every user-visible string lives in `lang/<code>.json`; the ui asks
//! for it with [`t`] (plain) or [`tf`] (with `{}` placeholders filled in order).
//! missing keys fall back to english, then to the key itself, so a half-translated
//! file never breaks the app.

use std::collections::HashMap;
use std::sync::{OnceLock, RwLock};

#[derive(
    serde::Deserialize, serde::Serialize, Clone, Copy, PartialEq, Eq, Debug, Default, strum::AsRefStr,
)]
pub enum Lang {
    #[default]
    English,
    Arabic,
}

impl Lang {
    pub const ALL: [Lang; 2] = [Lang::English, Lang::Arabic];

    fn table(self) -> &'static HashMap<&'static str, &'static str> {
        static EN: OnceLock<HashMap<&str, &str>> = OnceLock::new();
        static AR: OnceLock<HashMap<&str, &str>> = OnceLock::new();
        match self {
            Lang::English => EN.get_or_init(|| parse(include_str!("../../lang/en.json"))),
            Lang::Arabic => AR.get_or_init(|| parse(include_str!("../../lang/ar.json"))),
        }
    }

    /// the language's own name, for the dropdown
    pub fn name(self) -> &'static str {
        self.table().get("lang.name").copied().unwrap_or("?")
    }

    /// right-to-left script: the layout is mirrored for these
    pub fn is_rtl(self) -> bool {
        matches!(self, Lang::Arabic)
    }

    /// the language windows is set to, if we support it
    pub fn system() -> Lang {
        #[cfg(target_os = "windows")]
        {
            // primary language id (low 10 bits). LANG_ARABIC = 0x01
            let id = unsafe { windows::Win32::Globalization::GetUserDefaultUILanguage() };
            if id & 0x3ff == 0x01 {
                return Lang::Arabic;
            }
        }
        Lang::English
    }
}

fn parse(json: &'static str) -> HashMap<&'static str, &'static str> {
    // leak once at startup so `t()` can hand out &'static str; the tables are tiny
    let owned: HashMap<String, String> =
        serde_json::from_str(json).expect("lang file is not valid json");
    owned
        .into_iter()
        .map(|(k, v)| (&*k.leak(), &*v.leak()))
        .collect()
}

static CURRENT: RwLock<Lang> = RwLock::new(Lang::English);

pub fn set(lang: Lang) {
    *CURRENT.write().unwrap() = lang;
}

pub fn current() -> Lang {
    *CURRENT.read().unwrap()
}

/// is the current language right-to-left?
pub fn rtl() -> bool {
    current().is_rtl()
}

/// look up a string in the current language
pub fn t(key: &str) -> &'static str {
    let lang = current();
    lang.table()
        .get(key)
        .or_else(|| Lang::English.table().get(key))
        .copied()
        .unwrap_or_else(|| {
            log::warn!("missing translation: {key}");
            // leak so the fallback is 'static too; only happens for typos in keys
            String::from(key).leak()
        })
}

/// look up a string and fill its `{}` placeholders in order
pub fn tf(key: &str, args: &[&dyn std::fmt::Display]) -> String {
    let mut out = String::from(t(key));
    for arg in args {
        out = out.replacen("{}", &arg.to_string(), 1);
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn every_english_key_has_an_arabic_translation() {
        let en = Lang::English.table();
        let ar = Lang::Arabic.table();
        let missing: Vec<_> = en.keys().filter(|k| !ar.contains_key(*k)).collect();
        assert!(missing.is_empty(), "missing arabic keys: {missing:?}");
        let extra: Vec<_> = ar.keys().filter(|k| !en.contains_key(*k)).collect();
        assert!(extra.is_empty(), "arabic keys without english: {extra:?}");
    }

    #[test]
    fn placeholders_match() {
        let en = Lang::English.table();
        for (k, v) in Lang::Arabic.table() {
            assert_eq!(v.matches("{}").count(), en[k].matches("{}").count(), "placeholder count differs for {k}");
        }
    }

    #[test]
    fn tf_fills_in_order() {
        set(Lang::English);
        assert_eq!(tf("main.most_likely", &[&"a", &"b"]), "you are most likely to play on \"a\" (b)");
    }
}
