; ritrackdisklib-blank.adf: a blank DD floppy for the pass key floppy=N|SRC (the corpus runner):
; 80 cylinders x 2 sides x 11 sectors x 512 bytes = 901120 bytes of zeros, an ADF holds the
; decoded sectors, so the disk reads as 1760 valid sectors (not an unformatted disk; AmigaDOS
; sees no DOS disk).  Marks in sectors 0, 11 (track 1) and 1759 (the last) show that a read
; reaches the right place without a write before it.
s "SECTOR 0000"
fill 5621 0
s "SECTOR 0011"
fill 894965 0
s "SECTOR 1759"
fill 501 0
