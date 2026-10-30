; ricompactdisclib-opencd-name-overflow (#576) - OpenCD devicename$,unit
; copies the name into its 40-byte buffer without a limit; behind it lie
; the TOC pointer, its length, the TOC summary (12 bytes), the Q-code
; data (12 bytes, 60 bytes from the buffer's start), the request and the
; message port.  A name of 70 characters fills the Q-code data with "x"
; (the device does not open; the request and the port stay intact).
; CDTrackPlaying returns Q-code byte 1 (without checking for an open
; device, #586): "x" = 120.
; Correct: the long name does not touch the library's data:
; CDTrackPlaying returns 0 as before the call.
; @rwe ricompactdisclib
NPrint CDTrackPlaying
r.l=OpenCD(String$("x",70),0)
NPrint CDTrackPlaying
End
