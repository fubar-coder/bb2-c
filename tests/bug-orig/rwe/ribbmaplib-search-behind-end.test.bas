; ribbmaplib-search-behind-end (#553) - the chunk search reads the next
; chunk id and compares it before it tests the end of the file: after the
; last chunk the long behind the file is read and, if it is the id asked
; for, taken as a chunk.  The scenario (built in memory) is FORM SCEN
; with only a LEVL chunk (22 bytes in all); the bytes "DEST" lie directly
; behind it.
; Correct: BBDestructLoc gives 0 (the scenario has no DEST chunk); the
; LEVL chunk is still found (level 5).
DEFTYPE .l

!TestCase{dest_behind_the_file}
  m=AllocMem_(64,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,14 : Poke.l m+8,$5343454E
  Poke.l m+12,$4C45564C : Poke.l m+16,2 : Poke.w m+20,5
  Poke.l m+22,$44455354 : Poke.l m+26,4
  !CheckEq{SetBBScenarioLoc(m),-1,"scenario_set"}
  r=BBDestructLoc : If r Then r=r-m
  !CheckEq{r,0,"dest"}
  !CheckEq{BBScenarioLevel,5,"level"}
  FreeMem_ m,64
!EndTestCase
