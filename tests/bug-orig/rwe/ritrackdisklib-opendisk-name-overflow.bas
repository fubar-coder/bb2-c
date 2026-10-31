; ritrackdisklib-opendisk-name-overflow (#860) - OpenDisk unit,name$
; copies the name into its 58-byte buffer without a limit; behind the
; buffer lie the request pointers of units 0-3.  A name of 61 characters
; puts "xxx",0 into the request pointer of unit 0 (the device does not
; open, unit 3's own pointer is cleared).  finit closes units 0-2: it
; takes that pointer for an IOStdReq, calls DoIO on it and crashes (the
; session ends in a timeout).
; Correct: the long name does not touch the requests; the device does not
; open (0) and the program ends normally.
; @rwe ritrackdisklib
; @pass timeout=30
NPrint "before"
r.l=OpenDisk(3,String$("x",61))
NPrint r
NPrint "after"
End
