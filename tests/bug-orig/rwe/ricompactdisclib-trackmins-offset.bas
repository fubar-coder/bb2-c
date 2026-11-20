; ricompactdisclib-trackmins-offset (#586) - CDTrackMins/CDTrackSecs take
; an offset of 0 or 1 (the manual: "The optional parameter offset can take
; the value of 0 or 1"); another one reads the Q-code data 4 bytes per
; step behind (or before) the 12 bytes of the library, without a limit.
; The offset is an argument the programmer controls; the original has no
; error check area.  Here with runtime error checking.
; Correct: the error check refuses offset 2 after "before".
; @rwe ricompactdisclib
; @pass runtime_errors=on
NPrint "before"
NPrint CDTrackMins(2)
NPrint "after"
End
