; collslib-scoll-same-pair (#110) - SColl(k1,k2) of two sprite channels
; of the same pair (k1/2 = k2/2, e.g. 0 and 1) tests an unrelated bit of
; the latched CLXDAT (SColl(0,1): bit 8, a playfield-sprite collision),
; although the hardware reports no collision within a pair; the error
; check scollchk refuses only equal channels.
; An invalid argument: the error check catches it (BB2 style); the
; command itself is unchanged.  Runtime error checking on (the
; debugger).
; Correct: the error check stops SColl(0,1) with "Sprite channels must
; be of different pairs".  The original has no check and returns a
; value.
; The error is the tested behaviour; the message is judged by the requester
; (see the configuration), so it is not announced with !ExpectError.
DEFTYPE .l

!TestCase{same_pair}
  BLITZ
  Slice 0,44,320,200,$fff8,2,8,8,320,320
  DoColl
  a=SColl(0,1)
  AMIGA
  !Check{0,"after"}
!EndTestCase
