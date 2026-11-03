; compiler-macrobuff-freemem (#336) - blitz2 allocates macrobuff once at its
; start with the macro buffer size of then (8192), but frees it at its end
; with the size of then: options loaded later (an .xtra, here the defaults
; of tedstub with macrobuf=64) change macrolen, not the buffer. FreeMem of
; 64 bytes for the 8192 byte block leaves 8128 bytes lost (a larger size
; would free foreign memory, compiler-macrobuff-freemem-large). inject
; track reports the FreeMem.
; Correct: macrobuff is freed with the size it was allocated with.
; @pass wrap=@INJECT@|track
; @pass jobline=macrobuf=64
NPrint "hello"
End
