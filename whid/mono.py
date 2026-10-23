"""Fixed-width text for the tray label.

The AppIndicator extension draws the label in a plain St.Label, with the
panel's proportional bold font, and ignores any width hint. To keep the
icon still while a title scrolls, every character gets the same width:

  letters, digits  -> Unicode "Mathematical Monospace" (𝚁𝚎𝚞𝚗𝚒𝚘𝚗)
  accents          -> kept as combining marks over the letter (NFD)
  other characters -> padded with hair spaces up to the letter width
  whole text       -> padded at the end up to a fixed target width

Widths are measured with Pango, using the desktop font in bold, as the
GNOME panel does.
"""

import unicodedata

import gi

gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, Pango  # noqa: E402

HAIR_SPACE = " "
MONO_UPPER = 0x1D670  # MATHEMATICAL MONOSPACE CAPITAL A
MONO_LOWER = 0x1D68A  # MATHEMATICAL MONOSPACE SMALL A
MONO_DIGIT = 0x1D7F6  # MATHEMATICAL MONOSPACE DIGIT ZERO
WIDEST = "M"
SLACK_CELLS = 1  # Absorbs kerning and rounding between font runs.


def _glyph(c: str) -> str | None:
    # "R" -> "𝚁". None when no monospace variant exists.
    if "A" <= c <= "Z":
        return chr(MONO_UPPER + ord(c) - ord("A"))

    if "a" <= c <= "z":
        return chr(MONO_LOWER + ord(c) - ord("a"))

    if "0" <= c <= "9":
        return chr(MONO_DIGIT + ord(c) - ord("0"))

    return None


class Mono:
    def __init__(self) -> None:
        self.widget = Gtk.Label()  # Only provides a Pango context.
        name = Gtk.Settings.get_default().props.gtk_font_name
        self.font = Pango.FontDescription.from_string(name)
        self.font.set_weight(Pango.Weight.BOLD)

        self.cell = self._width(_glyph(WIDEST))
        self.hair = self._width(HAIR_SPACE)
        self.cache: dict[str, str] = {}  # Character -> padded form.

    def text(self, s: str, cells: int) -> str:
        # Same rendered width for any `s` of at most `cells` characters.
        out = "".join(self._char(c) for c in unicodedata.normalize("NFD", s))
        return self._pad(out, self.cell * (cells + SLACK_CELLS))

    def _char(self, c: str) -> str:
        glyph = _glyph(c)
        if glyph:
            return glyph

        if unicodedata.combining(c):
            return c

        if c not in self.cache:
            self.cache[c] = self._pad(c, self.cell)

        return self.cache[c]

    def _pad(self, s: str, target: float) -> str:
        if self.hair <= 0:
            return s  # No usable hair space in the font: give up.

        while self._width(s + HAIR_SPACE) <= target:
            s += HAIR_SPACE

        return s

    def _width(self, s: str) -> float:
        layout = self.widget.create_pango_layout(s)
        layout.set_font_description(self.font)
        return layout.get_size()[0] / Pango.SCALE
