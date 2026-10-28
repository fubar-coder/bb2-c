; t169_freakerr - displaylib: CustomString with a length that is no
; multiple of 4.  The original branches from the normal code into the
; error check area (freakerr): with runtime error checking the message
; (without it: behind the library - not run).
InitCopList 0,44,20,3,0,0,10
CustomString 0,0,0,"abcdefgh"
NPrint "ok"
CustomString 0,2,0,"abc"
NPrint "not reached"
End
