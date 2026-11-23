; riencryptlib-decrypt-mode1-carry (#629) - Encrypt steps its three
; wheels like an odometer, so after length bytes they are the 24 bit
; number "start + length".  Decrypt with mode 1 is to compute that end
; state from the start wheels, but adds the length's bytes to the wheels
; one by one, without the carries between them.  With wheel 1 = $FE and
; the length 300 ($12C) the carry into wheel 2 is lost: the end state is
; one step short and every byte comes out wrong.
; Correct: Decrypt mode 1 restores the block: the first byte is 0 and no
; byte is wrong.
DEFTYPE .l

!TestCase{mode_1_restores}
  Dim a.b(299)
  Encrypt &a(0),300,$FE,$FF,$7F
  Decrypt &a(0),300,$FE,$FF,$7F,1
  n=0
  For i=0 To 299
    If a(i)<>0 Then n+1
  Next
  !CheckEq{a(0),0,"first_byte"}
  !CheckEq{n,0,"bytes_wrong"}
!EndTestCase
