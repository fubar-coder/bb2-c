; rizonejoylib-no-table-results (#876) - without a zone table (init could
; not allocate table 0) SetZone returns -1, which is "done", and
; ZoneTable returns -1 instead of 0.  The program runs through inject,
; which fails the first allocation of 2052 bytes (init's table 0);
; ZoneTableSize 0 shows that there is no table.
; Correct: SetZone returns 0 (nothing was set), ZoneTable 0.
DEFTYPE .l

!TestCase{no_table}
  n=ZoneTableSize
  s=SetZone(1,5,5,9,9)
  t=ZoneTable
  !CheckEq{n,0,"size"}
  !CheckEq{s,0,"set_refused"}
  !CheckEq{t,0,"table_pointer"}
!EndTestCase
