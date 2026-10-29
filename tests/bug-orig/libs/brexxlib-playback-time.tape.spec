; brexxlib-playback-time: a tape (SaveTape/LoadTape format) with its start
; time and two events 2 s after it: RAWKEY "b" down and up
l 500 0                 ; start time: seconds, microseconds
b 1 0                   ; IECLASS_RAWKEY, subclass
w $35 0                 ; code, qualifier
l 0                     ; ie_EventAddress
l 502 0                 ; time stamp
b 1 0
w $B5 0
l 0
l 502 0
