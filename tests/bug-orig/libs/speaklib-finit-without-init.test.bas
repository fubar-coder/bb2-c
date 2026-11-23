; speaklib-finit-without-init (#278) - the init of speaklib opens
; translator.library, narrator.device and a signal for its reply port; if
; one fails it ends the program with the runtime error "Failed To Open
; Speech Library" (a TRAP in the init: runtime error checking on, without
; the debugger it is a Software Failure).  The finit then closes the
; device, frees the port's signal and closes the library without asking
; whether the init got that far: after a failed OpenDevice it calls
; CloseDevice on a request that has no device and FreeSignal(0), signal 0
; of the system, which the process never allocated.
; The program runs through inject: the OpenDevice of narrator.device
; fails (opendevfail=), a CloseDevice without a device and a FreeSignal
; of a signal not allocated are counted and not done (closedevice,
; freesignal), and the opens of translator.library are counted against
; its closes (opencount=).
; Here: the program uses VoiceLoc, so its init of speaklib runs and fails; the case behind it is never run,
; the observation is inject's report (see the configuration).
; Correct: the finit closes only what the init opened: the library (+0),
; no CloseDevice, no FreeSignal.
DEFTYPE .l

!TestCase{init_failed}
  v.l=VoiceLoc
  !Check{0,"not_reached"}
!EndTestCase
