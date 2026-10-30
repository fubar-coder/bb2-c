; risnowlib-initsnowdrop-no-free (#911) - InitSnowDrop allocates a new
; block for the lists of the field and overwrites the pointers of the
; list number without freeing the old block: every repeated call loses
; numpoints * 64 + 40 bytes (32040 for 500 points) until the end.  The
; program calls it three times and compares the free memory after the
; first and after the third call.
; Correct: the old block is freed, the free memory stays about the same.
; @rwe risnowlib
DEFTYPE .l
BitMap 0,320,256,3
lt=InitSnowDrop(0,500,0,10240)
a=AvailMem_(0)
lt=InitSnowDrop(0,500,0,10240)
lt=InitSnowDrop(0,500,0,10240)
b=AvailMem_(0)
If a-b>32000 Then NPrint "old blocks lost" Else NPrint "old blocks freed"
End
