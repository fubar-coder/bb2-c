; rievallib-ctp-dot-at-end (#631) - ConvToPostFix copies a "." inside an
; operand together with the next character, unchecked: at the end of the
; string that is its 0, and the scan goes on behind the string (here the
; bytes "(((7" behind a 0 inside the Blitz string).  The postfix text ends
; after "2.", and the conversion of the bytes behind the string follows
; it in the buffer.  Library 121 is the acidlibs' polylib on the CD: ref
; and port get the acidlibs of the variant rwe-1999-eval too (C build,
; without polylib).
; Correct: the 0 ends the operand: "1 2. + ", nothing behind it.
; @fixed #631
; @rwe rievallib
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|ref
; @pass file=blitz2/acidlibs|@CMAKE_BINARY_DIR@/variants/rwe-1999-eval/acidlibs|port
DEFTYPE .l
buf=AllocMem_(256,$10001)
If buf=0 Then NPrint "no memory" : End
ConvToPostFix "1+2."+Chr$(0)+"(((7",buf
NPrint "<",Peek$(buf),">"
n=0
For i=0 To 31
  If Peek.b(buf+i)<>0 Then n+1
Next
NPrint n," bytes not 0"
FreeMem_ buf,256
End
