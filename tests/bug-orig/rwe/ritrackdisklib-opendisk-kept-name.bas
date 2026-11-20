; ritrackdisklib-opendisk-kept-name (#859) - OpenDisk unit,name$ copies
; the name over the library's "trackdisk.device", and OpenDisk unit
; without a name opens the device of that name: after a failed OpenDisk
; 0,"nosuch.device" OpenDisk 0 fails too.
; Correct: OpenDisk unit always opens trackdisk.device; unit 0 (DF0)
; opens also without a disk in the drive.
; @rwe ritrackdisklib
; @fixed #859
NPrint OpenDisk(0,"nosuch.device")
NPrint OpenDisk(0)
NPrint CloseDisk(0)
End
