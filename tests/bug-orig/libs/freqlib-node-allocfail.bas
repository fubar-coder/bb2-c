; freqlib-node-allocfail (#159) - FileRequest$ does not check the AllocMem
; of a list node (40 bytes, MEMF_PUBLIC): when it fails, the next pointer,
; the type byte and the name are written from address 0 on (over the
; ExecBase pointer at 4).  The program makes a drawer CORPUS:fq159a with a
; file of a 34-character name (a node of 40 bytes, here as in the
; original) and opens the requester on it; it runs through inject, whose
; AllocMem of exactly 40 bytes with MEMF_PUBLIC fails (allocfail=), and
; which clicks CANCEL (click=) when the list has been read.
; Own 320x256 screen (see t93_freq).
; Correct: the entry is left out, CANCEL is answered and the program
; ends.  The original crashes.
; CORPUS: (dir= makes the drawer) is a host directory with long names;
; in RAM: of the emulator's system the original showed no overrun.
; @fixed #159
; @configs clib
; @pass dir=fq159a
; @pass runwrap=@INJECT@|allocfail=40-40/f=1 click=FQ/CANCEL
; @pass timeout=60
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
n$="CORPUS:fq159a/abcdefghijabcdefghijabcdefghijabcd"
If WriteFile(0,n$) Then CloseFile 0
p$="CORPUS:fq159a"
f$="x"
r$=FileRequest$("FQ",p$,f$)
NPrint "cancel ",Len(r$)
End
