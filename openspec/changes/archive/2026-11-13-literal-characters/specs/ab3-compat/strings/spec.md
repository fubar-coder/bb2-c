## ADDED Requirements

### Requirement: Latin-1 characters in strings and comments of an AmiBlitz3 text source
When `blitz2-ab3` loads an AmiBlitz3 text source in AB3 mode, it SHALL
turn every byte >= $80 inside a string literal and inside a comment into
a literal character (`$90 cc`, `editor/literal-characters`), including
the six umlauts, and not into the codes 26 to 31 (AB3's `gs` maps them
to other characters than Blitz 2's). The compiled string SHALL hold the
same bytes AmiBlitz3's compiler gives it, and a comment SHALL be skipped
as before. The loader SHALL leave bytes >= $80 outside strings and
comments as AB3's tokeniser leaves them.

#### Scenario: Umlaut and ß in a string
- **WHEN** an AB3 text source has `a$="Größe"` with ö = $F6 and ß = $DF and prints the string
- **THEN** the program prints the string with the bytes `47 72 F6 DF 65`

#### Scenario: Comment
- **WHEN** an AB3 text source has `; Größe` after a statement
- **THEN** the program compiles as without the comment

#### Scenario: Sources without such bytes
- **WHEN** an AB3 text source has no byte >= $80
- **THEN** its tokenised lines and its executable are byte-identical to the `prev` build's
