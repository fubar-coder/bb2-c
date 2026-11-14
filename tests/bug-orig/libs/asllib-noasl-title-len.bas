; asllib-noasl-title-len (#62) - without asl.library (the init could not
; open it) ASLFileRequest$ and ASLPathRequest$ return at once with d0 =
; the address of the title, which the compiler takes as the length of
; the result.  The program runs through inject, whose OpenLibrary of
; asl.library fails (openlibfail=), and takes only Len of each result
; (an assignment would copy the "string" of that length).
; Correct: both results are empty (length 0).  The original returns a
; length that is an address.
; @fixed #62
; @configs clib
; @pass runwrap=@INJECT@|openlibfail=asl.library
DEFTYPE .l
MaxLen p$=160
MaxLen f$=64
p$="RAM:" : f$="x"
l=Len(ASLFileRequest$("bo62",p$,f$))
If l=0 Then NPrint "file 0" Else NPrint "file not 0"
l=Len(ASLPathRequest$("bo62",p$))
If l=0 Then NPrint "path 0" Else NPrint "path not 0"
End
