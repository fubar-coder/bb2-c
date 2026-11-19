; rizonejoylib-no-table-results (#876) - without a zone table (init could
; not allocate table 0) SetZone returns -1, which is "done", and
; ZoneTable returns -1 instead of 0.  The program runs through inject,
; which fails the first allocation of 2052 bytes (init's table 0);
; ZoneTableSize 0 shows that there is no table.
; Correct: SetZone returns 0 (nothing was set), ZoneTable 0.
; @rwe rizonejoylib
; The program is silent for about 11 s (more than BUG_ORIG_RUN_HANG_AFTER):
; @pass hang_after=30
; @pass runwrap=@INJECT@|allocfail=2052-2052/1
NPrint ZoneTableSize
NPrint SetZone(1,5,5,9,9)
NPrint ZoneTable
End
