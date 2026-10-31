; rimisclib-initargparse-unbounded (#752) - InitArgParse copies the
; string with its 0 into its 256 byte buffer tempstorage and clears four
; bytes behind the copy, without a limit.  In the original the code of
; NextArgChar$ and NextArg$ follows the buffer: a string of 300
; characters overwrites it, the call of NextArgChar$ runs the text (a
; crash, the session ends in a timeout).
; Correct: InitArgParse copies at most 255 characters.
; @rwe rimisclib
; @pass timeout=30
DEFTYPE .l
b=AllocMem_(512,$10001)
If b=0 Then NPrint "no memory" : End
For i=0 To 299 : Poke.b b+i,120 : Next
InitArgParse b
NPrint "before"
a$=NextArgChar$(44)
n=Len(a$)
If n<=255 Then NPrint "at most 255" Else NPrint n
FreeMem_ b,512
End
