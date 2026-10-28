; audiolib-diskplay-endless (#69) - DiskPlay streams the BODY through two
; buffers of DiskBuffer bytes; loadbuff decrements the remaining length
; per buffer and reports the end only when it is below 0: at exactly 0 it
; returns "more", and the loop never ends.  The 8SVX here (generated) has
; a BODY of 2048 bytes, the buffer is 1024 bytes: the program hangs in
; DiskPlay (the audio DMA plays the last buffer again and again).
; Correct: DiskPlay returns after the 2048 bytes.
; @configs clib
; @pass timeout=45
; @pass file=t.8svx|@TESTDATA@/audiolib-diskplay-endless.8svx
DiskBuffer 1024
DiskPlay "CORPUS:t.8svx",1
NPrint "played"
End
