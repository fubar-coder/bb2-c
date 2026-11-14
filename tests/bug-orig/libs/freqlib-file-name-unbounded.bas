; freqlib-file-name-unbounded (#1042) - a click on a file line of
; FileRequest$ copies the name into the requester's file buffer (64
; bytes, the Buffer of the FILE: gadget) without a limit: a longer name
; runs on into the result buffer behind it, and OK then copies from the
; file buffer without ever finding a 0 (a runaway copy that crashes).
; The program makes a drawer CORPUS:fqname with a file of a 70-character
; name; inject clicks its line (the first 27 characters are visible on
; the 320 pixel screen), then CANCEL (click=), so that OK is not reached.
; Afterwards the program finds the requester's data block in its own
; code: the two unused bytes "::" in front of the path buffer, followed
; by the path "CORPUS"; the literal below is not preceded by "::"; and
; counts the characters from the file buffer (path buffer + 240) up to a
; 0, at most 512.
; pad: in the original every name of more than 34 characters is written
; behind its 40-byte list node (#159); pad's 64 zeroed bytes behind each
; allocation take that overrun, so the original's run is deterministic.
; Correct: a name that does not fit the 64-byte buffer is refused, the
; buffer keeps "x".  The original copies all 70 characters.
; @fixed #1042
; @configs clib
; @pass dir=fqname
; @pass runwrap=@INJECT@|pad click=FQ/a123456789b123456789c123456 click=FQ/CANCEL
; @pass timeout=120
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
n$="CORPUS:fqname/a123456789b123456789c123456789d123456789e123456789f123456789g123456789"
If WriteFile(0,n$) Then CloseFile 0
p$="CORPUS:fqname"
f$="x"
r$=FileRequest$("FQ",p$,f$)
NPrint "cancel ",Len(r$)
s.l=?here
b.l=0
i.l=-60000
While i<60000
If Peek.l(s+i)=$3A3A434F AND Peek.l(s+i+4)=$52505553 Then b=s+i+2 : i=70000
i+2
Wend
here:
If b=0 Then NPrint "data block not found" : End
n.l=0
While Peek.b(b+240+n)<>0 AND n<512
n+1
Wend
NPrint "file buffer (64 bytes): ",n," characters"
End
