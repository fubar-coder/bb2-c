; ricompactdisclib.cue: the CD image of the tests of ricompactdisclib (one
; image per library, tests/README.md "Adding a test"): two audio tracks of
; zeros (300 and 450 sectors of 2352 bytes), for uaescsi.device
; (uae=scsi=true)
s "FILE \"ricompactdisclib-t1.bin\" BINARY\n"
s "  TRACK 01 AUDIO\n"
s "    INDEX 01 00:00:00\n"
s "FILE \"ricompactdisclib-t2.bin\" BINARY\n"
s "  TRACK 02 AUDIO\n"
s "    INDEX 01 00:00:00\n"
