; const-novalue - a constant defined without a value (a line "#c", no '=')
; is 0, CNIF is false; "#d=5" then "#d" keeps 5 (#972, the same in
; AmiBlitz3)
OPTIMIZE 4
!TestCase{const_novalue}
  #c
  #d=5
  #d
  !CheckEq{#c,0,"l1"}
  zs$=""
  CNIF #c
    zs$="cnif true"
  CELSE
    zs$="cnif false"
  CEND
  !CheckStrEq{zs$,"cnif false","l2"}
  !CheckEq{#d,5,"l3"}
!EndTestCase
