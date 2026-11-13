## MODIFIED Requirements

### Requirement: Display, help and colour
The editor SHALL draw a literal character as the glyph of its code (the
font of `editor/screen-font`) in the colour of ordinary text (not the
token colour) and SHALL NOT offer help for it. The glyph SHALL be the same
whether the cell was drawn by typing, by loading, or by a redraw (scroll,
refresh, reline), also in rows other than the cursor row. A character the
font has no glyph for SHALL be drawn as the font's default glyph (glyph 0);
the stored pair SHALL NOT change. With the font of this change no
character that can be stored has no glyph.

#### Scenario: Colour
- **WHEN** a line with a literal character is shown with tokenise on
- **THEN** the character has the colour of the text next to it

#### Scenario: Help key on a literal character
- **WHEN** the cursor is on a literal character and the help key is pressed
- **THEN** no token help is shown

#### Scenario: The glyph of the code
- **WHEN** a line holds `$90 DF` (`ß`) and the line is shown
- **THEN** the cell shows the glyph of `ß`, not glyph 0

#### Scenario: Redraw outside the cursor row
- **WHEN** a line with `é` is not the cursor row and the screen scrolls horizontally or is refreshed
- **THEN** the cell still shows the glyph of `é`
