; menuslib-numbers-unchecked (#416) - MenuState MenuList#,Menu,Item,Off
; follows the next pointers Item+1 times without a test for the end of
; the list (only the check routines of the debugger test it): an item
; beyond the end follows the 0 pointer to address 0, then the long found
; there, and clears bit 4 of byte 13 of where it ends.
; Here the long at address 0 is set to a buffer of the program that
; points to itself (restored afterwards), so the walk ends in the buffer
; and not in the exception vectors; menu list 0 has 1 item, MenuState
; on item 5.
; Correct: memory outside the menu list is not changed (byte 13 of the
; buffer stays $FF).
; By design (#416): the original (and the port) test the numbers only with
; runtime error checking, so this test comes out REPRODUCED.
DEFTYPE .l

!TestCase{item_beyond_end}
  MenuTitle 0,0,"Project"
  MenuItem 0,0,0,0,"Load"
  b=AllocMem(64,$10001)
  Poke.l b,b : Poke.b b+13,$FF
  z=Peek.l(0)
  Poke.l 0,b
  MenuState 0,0,5,Off
  Poke.l 0,z
  !CheckEq{Peek.b(b+13) AND $FF,$FF,"buffer_byte_13"}
  FreeMem b,64
!EndTestCase
