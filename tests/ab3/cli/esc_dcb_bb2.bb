; BB2 mode: a Dc.b string keeps every byte as written, its escapes are
; AB3 mode's (spec ab3-compat/strings)
NPrint Peek$(?v)
End
.v
Dc.b "V\\__VER_MAJOR.\\__VER_MINOR [\\41] [\\__DATE__]",0
