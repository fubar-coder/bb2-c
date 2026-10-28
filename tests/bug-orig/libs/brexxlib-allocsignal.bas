; brexxlib-allocsignal (#104) - codeinit takes the result of
; AllocSignal(-1) unchecked as the "playback finished" signal: without a
; free signal the bit is -1 and the mask 0 (LSL.l by 63), PlayWait waits
; with Wait(0) and never returns.  The program runs through inject
; without a free signal (runwrap=, inject signals); a tape with two
; events is played back and waited for.  With free signals (normal
; start) the program prints "played 0".
; Correct (one fix of the issue: PlayWait returns without a signal):
; PlayWait returns, the program ends.  The original hangs in PlayWait.
; @configs clib
; @pass runwrap=@INJECT@|signals
; @pass timeout=45
QuietTrap On
TapeTrap 0
MouseButton 0,1
MouseButton 0,0
QuietTrap Off
PlayBack 0
NPrint "status ",XStatus
PlayWait 0
NPrint "played ",XStatus
End
