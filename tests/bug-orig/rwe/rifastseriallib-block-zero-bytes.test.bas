; rifastseriallib-block-zero-bytes (#646) - the block forms of PutSerial,
; GetSerial, SyncPut and SyncGet send/receive one byte and then count
; with SUBQ.l #1 / BNE: 0 bytes are 2^32.  PutSerial address,0 sends the
; memory from the buffer on, 4 GB at 19200 baud - the program does not
; come back (the session ends in a timeout).
; Correct: a block of 0 bytes sends nothing, the program goes on.
DEFTYPE .l

!TestCase{block_of_zero}
  b=AllocMem_(16,$10001)
  !Require{b<>0,"memory"}
  r=InitSerialIO(19200,16,b,b)
  n=0
  PutSerial b,n
  !Check{1,"returned"}
  FinitSerialIO
  FreeMem_ b,16
!EndTestCase
