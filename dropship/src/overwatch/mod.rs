use crate::api;

pub const PROCESS_NAME: &str = "Overwatch.exe";

#[derive(Default, Clone, Copy, serde::Serialize, serde::Deserialize, Debug)]
pub struct ServerSelection(u64);

impl ServerSelection {
    pub fn has_bit(&self, bit: u8) -> bool {
        debug_assert!(bit < 64);

        (self.0 & (1u64 << bit)) != 0
    }

    pub fn has(&self, server: &api::KnownServer) -> bool {
        self.has_bit(server.bit)
    }

    pub fn set_bit(&mut self, bit: u8) {
        debug_assert!(bit < 64);

        self.0 |= 1u64 << bit;
    }

    #[allow(dead_code)]
    pub fn set(&mut self, server: &api::KnownServer) {
        self.set_bit(server.bit)
    }

    #[allow(dead_code)]
    pub fn clear_bit(&mut self, bit: u8) {
        debug_assert!(bit < 64);

        self.0 &= !(1u64 << bit);
    }

    #[allow(dead_code)]
    pub fn clear(&mut self, server: &api::KnownServer) {
        self.clear_bit(server.bit)
    }

    pub fn toggle_bit(&mut self, bit: u8) {
        debug_assert!(bit < 64);

        self.0 ^= 1u64 << bit;
    }

    pub fn toggle(&mut self, server: &api::KnownServer) {
        self.toggle_bit(server.bit);
    }

    pub fn solo_bit(&mut self, bit: u8) {
        debug_assert!(bit < 64);

        self.0 = 1u64 << bit;
    }

    pub fn solo(&mut self, server: &api::KnownServer) {
        self.solo_bit(server.bit);
    }

    pub fn solo_invert(&mut self, server: &api::KnownServer) {
        self.solo_bit(server.bit);
        self.invert();
    }

    pub fn invert(&mut self) {
        self.0 = !self.0;
    }

    pub const fn none() -> Self {
        Self(0)
    }

    pub const fn bits(&self) -> u64 {
        self.0
    }
}
