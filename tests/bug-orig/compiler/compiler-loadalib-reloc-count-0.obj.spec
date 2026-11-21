; compiler-loadalib-reloc-count-0: a hand-made Blitz library file, the
; library of amiga/userlib.c (number 10, one function BugOrigUserLib that
; returns 42) with an empty HUNK_RELOC32: a count of 0 right behind its
; id, then HUNK_END.  The library needs no relocation in its code (its
; only reloc, the address of the function in its structure, lies before
; the code and is never applied: uselib3 skips relocs below maxinfo).
; loadalib reads the 12 bytes behind the structure as the reloc header:
; $3EC, the count 0, $3F2.
l $3f3 0 1 0 0          ; HUNK_HEADER: no names, one hunk (0..0)
l 26                    ; its size in long words
l $3e9 26               ; HUNK_CODE, 26 long words (104 bytes)
; the 104 bytes of the code hunk as the build of userlib.c has them:
; the library structure (number 10, the entry of BugOrigUserLib with
; its name) and behind it the code of douserlib (moveq #42,d0 / rts)
w $7000 $4e75 10
fill 42 0
w $0302
fill 14 0
w $0064 0 0 $ffff 0 0 0
s "BugOrigUserLib"
w 0 $ffff 0 0
w $702a $4e75
l $3ec 0                ; HUNK_RELOC32 with a count of 0 (no reloc data)
l $3f2                  ; HUNK_END
