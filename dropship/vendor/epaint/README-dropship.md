# vendored epaint 0.36.1 (+ bidi)

This is `epaint 0.36.1` from crates.io with one change in `src/text/text_layout.rs`:
UAX#9 bidirectional layout (via `unicode-bidi`) so right-to-left scripts such as
Arabic are shaped in the right direction, ordered correctly next to Latin text and
digits, and wrapped in logical order.

Search for `dropship-ar` in `text_layout.rs` to find every change. Remove this
directory and the `[patch.crates-io]` entry in `Cargo.toml` once egui ships
bidi support upstream (https://github.com/emilk/egui/issues/1016).
