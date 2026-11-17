; riamosfunclib-bload-open-fails (#487) - BLoad into a bank erases the
; bank and reserves it anew with the length before it opens the file: a
; file that cannot be opened leaves a bank in use erased and reserved with
; the new length, and a free bank reserved.
; Correct: BLoad returns 0 and both banks stay as they were (32 bytes,
; free).
; @fixed #487
; @rwe riamosfunclib
DEFTYPE .l
r=Reserve(11,32)
r=BLoad("RAM:riamosfunclib-no-such-file",11,16)
NPrint r
NPrint Length(11)
r=BLoad("RAM:riamosfunclib-no-such-file",12,16)
NPrint Length(12)
End
