; sortlib-string-null (#276) - Sort on a string array reads the length of
; each element at -4(pointer); an element that was never assigned has the
; pointer 0, so its length comes from address -4 and its characters from
; address 0: the order is arbitrary.  To make the characters at address 0
; count, the program puts a "z" there during the sorts (and restores the
; byte): an element with the pointer 0 then sorts as a string beginning
; with "z" if its length is not 0.
; Correct: a never assigned element is the empty string and sorts first
; (ascending) or last (SortDown).
; The emulator runs with 24 bit addressing (as a 68000 or 68EC020 Amiga):
; there address -4 is the last long of the Kickstart ROM, not 0.
DEFTYPE .l

!TestCase{unassigned_elements}
  Dim a$(3)
  a$(0)="b" : a$(2)="a"
  z.b=Peek.b(0) : Poke.b 0,Asc("z")
  Sort a$()
  Poke.b 0,z
  !CheckStrEq{a$(0),"","up0"}
  !CheckStrEq{a$(1),"","up1"}
  !CheckStrEq{a$(2),"a","up2"}
  !CheckStrEq{a$(3),"b","up3"}
  Dim b$(3)
  b$(1)="a" : b$(3)="b"
  SortDown
  Poke.b 0,Asc("z")
  Sort b$()
  Poke.b 0,z
  !CheckStrEq{b$(0),"b","down0"}
  !CheckStrEq{b$(1),"a","down1"}
  !CheckStrEq{b$(2),"","down2"}
  !CheckStrEq{b$(3),"","down3"}
!EndTestCase
