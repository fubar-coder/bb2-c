; freqlib-node-overrun (#159) - FileRequest$ keeps each entry of its
; list in a 40-byte node (AllocMem(40)): next pointer, type byte, then the
; name up to its 0 without a limit, so a name of more than 34 characters
; is written behind the node.  The program makes a drawer CORPUS:fq159 with
; a file of a 40-character name and opens the requester on it; it runs
; through inject with guard (a pattern behind every allocation, checked
; at FreeMem), which clicks CANCEL (click=) when the list has been read.
; Own 320x256 screen (see t93_freq).
; Correct: nothing is written behind an allocation.  The original writes
; 6 bytes behind the 40-byte node of the file.
; CORPUS: (dir= makes the drawer) is a host directory with long names;
; in RAM: of the emulator's system the original showed no overrun.
; @fixed #159
; @configs clib
; @pass dir=fq159
; @pass runwrap=@INJECT@|guard click=FQ/CANCEL
; @pass timeout=120
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
n$="CORPUS:fq159/abcdefghijabcdefghijabcdefghijabcdefghij"
If WriteFile(0,n$) Then CloseFile 0
p$="CORPUS:fq159"
f$="x"
r$=FileRequest$("FQ",p$,f$)
NPrint "cancel ",Len(r$)
End
