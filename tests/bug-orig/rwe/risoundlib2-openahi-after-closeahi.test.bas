; risoundlib2-openahi-after-closeahi (#927) - CloseAHI is finit: it deletes
; the I/O request and the port, and nothing makes them again.  A later
; OpenAHI writes ahir_Version through the request pointer, now 0 - the
; word 4 to address 48 - and calls OpenDevice with request 0.  Address 48
; (an unused exception vector) holds a marker during the second OpenAHI;
; it and the longs at 20 and 28 (io_Device, io_Error of a request at 0)
; are put back behind it.
; Correct: the second OpenAHI fails without writing to address 48.
DEFTYPE .l

!TestCase{openahi_after_closeahi}
  r=OpenAHI(0)
  CloseAHI
  o48=Peek.l(48) : o20=Peek.l(20) : o28=Peek.l(28)
  Poke.l 48,$12345678
  r2=OpenAHI(0)
  m=Peek.l(48)
  Poke.l 48,o48 : Poke.l 20,o20 : Poke.l 28,o28
  k=(m=$12345678)
  !CheckEq{k,-1,"address_48_kept"}
  !CheckEq{r2,0,"open_again"}
!EndTestCase
