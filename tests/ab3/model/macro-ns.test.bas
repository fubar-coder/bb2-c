; macro-ns - macro calls without braces (new syntax, OPTIMIZE bit 4; spec
; ab3-compat/syntax-rules): a macro whose text has a parameter takes them
; from behind its name; one without takes none. OPTIMIZE 16 replaces the
; flags: new syntax off, braces again.
Macro pm
zs$="<`1|`2>"
End Macro
Macro pq
zs$=`1
End Macro
Macro np
zs$="np"
End Macro
#zc = 6
!TestCase{macro_ns}
  OPTIMIZE 4
  !pm 42
  !CheckStrEq{zs$,"<42|>","l1"}
  !pm{7}
  !CheckStrEq{zs$,"<7|>","l2"}
  !pm 3,4
  !CheckStrEq{zs$,"<3|4>","l3"}
  !pm -5
  !CheckStrEq{zs$,"<-5|>","l4"}
  !pm $10
  !CheckStrEq{zs$,"<$10|>","l5"}
  !pm #zc
  !CheckStrEq{zs$,"<#zc|>","l6"}
  !pq "s,t"
  ; the expected text with Chr$(44): no comma in a check macro argument
  !CheckStrEq{zs$,"s"+Chr$(44)+"t","l7"}
  !pm
  !CheckStrEq{zs$,"<|>","l8"}
  !pm :zc$="colon"
  !CheckStrEq{zs$,"<|>","l9"}
  !CheckStrEq{zc$,"colon","l10"}
  !np :zc$="np colon"
  !CheckStrEq{zs$,"np","l11"}
  !CheckStrEq{zc$,"np colon","l12"}
  OPTIMIZE 16
  !pm{8,9}
  !CheckStrEq{zs$,"<8|9>","l13"}
!EndTestCase
