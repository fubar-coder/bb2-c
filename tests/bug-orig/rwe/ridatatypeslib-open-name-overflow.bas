; ridatatypeslib-open-name-overflow (#604) - DataTypeOpen copies the file
; name into its 384-byte buffer (fname) without a limit, before it locks
; the file; behind the buffer lies the BitMapHeader pointer (in both
; builds).  The name here has 387 characters: the last three and the 0
; become the pointer.  They are chosen as the address of a fake
; BitMapHeader (width 1234) in the program's memory (an address whose
; three upper bytes are not 0 and whose lowest is).  The file does not
; exist (Lock fails).
; Correct: the name does not change the library's state - DataTypeWidth
; has no BitMapHeader and returns 0.
; @rwe ridatatypeslib
DEFTYPE .l
b=AllocMem_($30000,$10004)
If b=0 Then NPrint "no memory" : End
a=(b+255) AND $FFFFFF00
While ((a/256) AND 255)=0 OR ((a/65536) AND 255)=0
  a=a+256
Wend
Poke.w a,1234
n$=String$("x",384)+Chr$(a/16777216)+Chr$((a/65536) AND 255)+Chr$((a/256) AND 255)
NPrint "name ",Len(n$)
NPrint "open ",DataTypeOpen(n$)
NPrint "width ",DataTypeWidth
FreeMem_ b,$30000
End
