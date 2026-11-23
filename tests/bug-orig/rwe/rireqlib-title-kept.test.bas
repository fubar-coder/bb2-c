; rireqlib-title-kept (#789) - ReqFileRequest(title$) keeps the pointer
; to the title string, which it gives up to the string work area, for
; the later calls without a title: the second call's requester gets the
; title field of the first call, a pointer into the string work area
; that no longer holds the title.  With the real req.library 2.8
; (Aminet util/libs/ReqLib28.lha, skipped without it) the second
; ReqFileRequest halts the emulated CPU in the original (the session
; ends as `halt`).  The program runs through inject, which puts the
; library into LIBS: (libs=) and answers each requester with its cancel
; gadget (click=, `*` for the title: the window with the gadget; it
; writes the title the window had).
; Correct: the title is for that call only; the second requester has the
; title of the empty title buffer, "".
DEFTYPE .l

!TestCase{second_call}
  a$=ReqFileRequest("First title")
  b$=ReqFileRequest
  !Check{1,"done"}
!EndTestCase
