; ridoslib-addnotify-id-reused (#620) - AddNotify stores the new request
; in its table without looking at the entry: a request already there for
; that id stays active but can no longer be ended.  Id 1 watches
; CORPUS:bug-orig-620-a, then CORPUS:bug-orig-620-b; RemNotify 1 ends only
; the second.  Then CORPUS:bug-orig-620-a is written: the lost request
; still sends its message (id 1).
; Correct: after RemNotify 1 no request is left, no message arrives (0).
DEFTYPE .l

!TestCase{lost_request}
  r=AddNotify(1,"CORPUS:bug-orig-620-a")
  r=AddNotify(1,"CORPUS:bug-orig-620-b")
  RemNotify 1
  f=Open_("CORPUS:bug-orig-620-a",1006)
  !Require{f<>0,"file_written"}
  Close_ f
  Delay_ 25
  !CheckEq{CheckNotify,0,"messages"}
!EndTestCase
