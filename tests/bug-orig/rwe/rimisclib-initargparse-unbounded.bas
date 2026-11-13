; rimisclib-initargparse-unbounded (#752) - InitArgParse copies the
; string with its 0 into its 256 byte buffer tempstorage and clears four
; bytes behind the copy, without a limit.  In the original the code of
; NextArgChar$ follows the buffer.  The string here is 256 characters and
; then the four bytes $70 $2A $4E $75: they land on the entry of
; NextArgChar$ and are MOVEQ #42,d0 : RTS, so the original returns a
; string of 42 characters whatever the memory layout is (text that is
; run as it comes, e.g. 300 times "x", ends in a crash or runs through,
; depending on the address the library was loaded to).
; Correct: InitArgParse copies at most 255 characters.
; @fixed #752
; @rwe rimisclib
; @pass timeout=30
DEFTYPE .l
b=AllocMem_(512,$10001)
If b=0 Then NPrint "no memory" : End
For i=0 To 255 : Poke.b b+i,120 : Next
Poke.b b+256,$70 : Poke.b b+257,$2A : Poke.b b+258,$4E : Poke.b b+259,$75
InitArgParse b
NPrint "before"
a$=NextArgChar$(44)
If Len(a$)=255 Then NPrint "the first 255 characters" Else NPrint "not the first 255 characters"
FreeMem_ b,512
End
