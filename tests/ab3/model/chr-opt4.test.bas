; chr-opt4 - character literals of AB3 mode under OPTIMIZE 4 (spec
; ab3-compat/strings).
OPTIMIZE 4
!TestCase{chr_value}
  ; with new syntax, @"..." is an integer of the character codes, the first
  ; one highest; up to 1, 2 or 4 characters for a byte, word or long
  ; target, @"" is 0; a comparison, Select/Case and an expression as
  ; AmiBlitz3's includes use
  a.l = @"ILBM"
  !CheckStrEq{Hex$(a),"494C424D","l1"}
  a = @"AB"
  !CheckStrEq{Hex$(a),"00004142","l2"}
  a = @""
  !CheckStrEq{Hex$(a),"00000000","l3"}
  cv_b.b = @"A"
  !CheckEq{cv_b,65,"l4"}
  cv_b = @""
  !CheckEq{cv_b,0,"l5"}
  w.w = @"AB"
  !CheckStrEq{Hex$(w),"00004142","l6"}
  w = @"Z"
  !CheckEq{w,90,"l7"}
  id.l = @"RIFF" : zs$="" : If id = @"RIFF" AND id <> @"FORM" Then zs$="cmp"
  !CheckStrEq{zs$,"cmp","l8"}
  zs$=""
  Select id
    Case @"FORM"
      zs$="form"
    Case @"RIFF"
      zs$="riff"
  End Select
  !CheckStrEq{zs$,"riff","l9"}
  n.w = 7
  !CheckStrEq{Chr$(n + @"0"),"7","l10"}
  !CheckEq{@"0" + 5,53,"l11"}
!EndTestCase
!TestCase{chr_mem}
  ; the value of @"ILBM" holds the characters in their written order, as
  ; Peek.b sees them and as WriteMem writes them to a file (read back byte
  ; by byte); the same for a word
  a = @"ILBM" : zs$=Chr$(Peek.b(&a))+Chr$(Peek.b(&a + 1))+Chr$(Peek.b(&a + 2))+Chr$(Peek.b(&a + 3))
  !CheckStrEq{zs$,"ILBM","l1"}
  w = @"AB" : zs$=Chr$(Peek.b(&w))+Chr$(Peek.b(&w + 1))
  !CheckStrEq{zs$,"AB","l2"}
  If OpenFile(0, "RAM:chr_mem.bin")
    WriteMem 0, &a, 4
    CloseFile 0
  EndIf
  b.l = 0
  zs$=""
  If ReadFile(0, "RAM:chr_mem.bin")
    zs$="length "+Str$(Lof(0))
    ReadMem 0, &b, 4
    CloseFile 0
  EndIf
  !CheckStrEq{zs$,"length 4","l3"}
  zs$=""
  For i.w = 0 To 3
    zs$=zs$+Right$(Hex$(Peek.b(&b + i) & $FF), 2)+" "
  Next
  !CheckStrEq{zs$,"49 4C 42 4D ","l4"}
!EndTestCase
