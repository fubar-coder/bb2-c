; risoundlib2-openahi-after-closeahi (#927) - CloseAHI is finit: it deletes
; the I/O request and the port, and nothing makes them again.  A later
; OpenAHI writes ahir_Version through the request pointer, now 0 - the
; word 4 to address 48 - and calls OpenDevice with request 0.  Address 48
; (an unused exception vector) holds a marker during the second OpenAHI;
; it and the longs at 20 and 28 (io_Device, io_Error of a request at 0)
; are put back behind it.
; Correct: the second OpenAHI fails without writing to address 48.
; @rwe risoundlib2
; @pass timeout=30
DEFTYPE .l
r=OpenAHI(0)
NPrint "open ",r
CloseAHI
o48=Peek.l(48) : o20=Peek.l(20) : o28=Peek.l(28)
Poke.l 48,$12345678
r=OpenAHI(0)
m=Peek.l(48)
Poke.l 48,o48 : Poke.l 20,o20 : Poke.l 28,o28
If m=$12345678 Then NPrint "address 48 kept" Else NPrint "address 48 written"
NPrint "open again ",r
End
