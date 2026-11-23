; ilbmifflib-bitmap-ignores-init (#187) - ilbmbitmap (LoadBitMap without
; a palette, LoadScreen) calls ilbminit and searches "BODY" without
; looking at its result: for a file that cannot be opened it searches in
; what is left in iffiolib's buffer (with an ILBM still in the buffer the
; original takes the size from a0, which dos.library left behind, and
; the port from the last ILBM: a picture of the old file or garbage).
; Here the first IFF command of the program, LoadBitMap of a file that
; does not exist: the buffer is empty, the search for "BODY" fails at
; once, and the message is "Can't Find IFF 'BODY'".
; Correct: a file that cannot be opened is the runtime error "Can't Open
; File" (the library's message that the original never uses).  The message
; is the tested behaviour, so the error is not announced with
; !ExpectError (see the configuration).
DEFTYPE .l

!TestCase{missing_file}
  BitMap 1,16,4,1
  LoadBitMap 1,"CORPUS:bo187.missing"
  !Check{0,"loaded"}
!EndTestCase
