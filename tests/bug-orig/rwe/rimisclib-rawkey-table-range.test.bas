; rimisclib-rawkey-table-range (#744) - ASCRaw and RawASC use a table of
; 114 raw key codes without checking its range: ASCRaw of a key up code
; (128-255) or of 114 and more returns a byte of the code behind the
; table, RawASC of a character not in the table (an upper case letter)
; searches on behind it until it finds the byte somewhere, and RawASC("")
; finds the 0 of a key without a character (14).
; Correct: ASCRaw outside 0..113 gives the character 0, RawASC of a
; character not in the table and of "" gives -1.
DEFTYPE .l

!TestCase{table_range}
  a=Asc(ASCRaw(32))
  b=Asc(ASCRaw(160))
  c=Asc(ASCRaw(200))
  d=RawASC("a")
  e=RawASC("A")
  f=RawASC("")
  !CheckEq{a,97,"ascraw_32"}
  !CheckEq{b,0,"ascraw_160"}
  !CheckEq{c,0,"ascraw_200"}
  !CheckEq{d,32,"rawasc_a"}
  !CheckEq{e,-1,"rawasc_upper_a"}
  !CheckEq{f,-1,"rawasc_empty"}
!EndTestCase
