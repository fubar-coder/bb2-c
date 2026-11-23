; freqlib-libs-not-closed (#154) - FileRequest$ opens dos, intuition and
; graphics (OldOpenLibrary) at every call and never closes them.  The
; program runs through inject, which counts the opens minus the closes of
; the process (opencount=) and answers the requester with its CANCEL
; gadget (click=).  Own 320x256 screen: on the emulator's tall Workbench
; the requester writes past its data block (see t93_freq).
; Correct: every library the process opened is closed again (+0).  The
; original leaves each of the three opened once more (inject's report,
; asserted on `inject`).
DEFTYPE .l

!TestCase{opencount}
  Screen 0,0,0,320,256,2,0,"freqlib",1,2
  Use Screen 0
  MaxLen p$=160
  MaxLen f$=64
  p$="RAM:"
  f$="x"
  r$=FileRequest$("FQ",p$,f$)
  !CheckEq{Len(r$),0,"cancel"}
!EndTestCase
