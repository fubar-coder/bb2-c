; long_umlaut - TED test source (stage 2b): a line over 127 characters,
; Latin-1 umlauts äöü ÄÖÜ ß in a comment and in strings, $FD-$FF
a$="äöüß" : NPrint a$
NPrint "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789"
b$="ışÿ" : NPrint Len(b$)
End
