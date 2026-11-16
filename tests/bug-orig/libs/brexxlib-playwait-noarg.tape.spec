; brexxlib-playwait-noarg: a tape (SaveTape/LoadTape format) with its
; start time and two POINTERPOS events 1 and 2 s after it
l 500 0                 ; start time: seconds, microseconds
b 4 0                   ; IECLASS_POINTERPOS, subclass
w 0 0                   ; code, qualifier
w 40 30                 ; x, y
l 501 0                 ; time stamp
b 4 0
w 0 0
w 60 40
l 502 0
