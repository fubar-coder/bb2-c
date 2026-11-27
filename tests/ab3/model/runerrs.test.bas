; XTRA
; Embedded .xtra Header
;
; Debugger
; -------------------------------------------------------
; RuntimeDebug    = 1
; /XTRA
; runerrs - RunErrsOff nests, LastRunErrsMode goes back to the mode before
; the matching RunErrsOff: a(9) is out of range, checked only at the end
; (the requester, asserted in runerrs.test.toml; AmiBlitz3 runs without
; its debugger and does not check it)
!TestCase{runerrs}
  Dim a(5)
  RunErrsOff
  x = a(9)
  !CheckStrEq{"off 1","off 1","l1"}
  RunErrsOff
  x = a(9)
  !CheckStrEq{"off 2","off 2","l2"}
  LastRunErrsMode
  x = a(9)
  !CheckStrEq{"off 3","off 3","l3"}
  LastRunErrsMode
  !CheckStrEq{"on","on","l4"}
  x = a(9)
  !CheckStrEq{"not checked","not checked","l5"}
!EndTestCase
