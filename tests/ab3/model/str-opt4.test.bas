; str-opt4 - strings of AB3 mode under OPTIMIZE 4 (spec ab3-compat/strings).
; The file holds Latin-1 bytes (latin1_string): keep it in Latin-1.
OPTIMIZE 4
; str_expr: escapes in string expressions: literals with escapes in a
; chain of +, in the six comparisons and as operands of AND and OR, in the
; string parameters of a Function (in registers: the filler below the
; length, AmiBlitz3's pea 0) and of a library function, a string Function
; with escapes in a comparison, and two string expressions in one statement
Function.s t{a.s,b.s}
Function Return a+"\\2d"+b
End Function
Function.l n{a.s,b.l}
Function Return Len(a)*10+b
End Function
!TestCase{str_expr}
  a$="\\41b" : b$="c\\44"
  c$=a$+"\\N"+b$+"|"+"\\\\"+UCase$("\\61x")+"\\5a"
  k.l=Instr(c$,Chr$(10)) : zs$=Left$(c$,k-1)
  !CheckStrEq{zs$,"Ab","l1"}
  zs$=Mid$(c$,k+1)+" "+Str$(Len(c$))
  !CheckStrEq{zs$,"cD|\\\\AXZ 11","l2"}
  zs$=Str$(a$="Ab")+" "+Str$(a$<>"\\41b")+" "+Str$(a$<b$)
  zs$=zs$+" "+Str$(a$<="\\41b")+" "+Str$(b$>a$)+" "+Str$(b$>="cE")
  !CheckStrEq{zs$,"-1 0 -1 -1 -1 0","l3"}
  r.l=a$="\\41b" AND b$="c\\44"
  !CheckEq{r,-1,"l4"}
  r=a$="x" OR b$+"\\21"="cD!"
  !CheckEq{r,-1,"l5"}
  zs$=t{a$+"\\31",b$}
  !CheckStrEq{zs$,"Ab1-cD","l6"}
  zs$=t{"\\5b",t{"\\5d","\\7c"}}
  !CheckStrEq{zs$,"[-]-|","l7"}
  zs$=Str$(n{"\\41\\42",3})+" "+Str$(1+(2+n{a$+"\\43",4}))
  !CheckStrEq{zs$,"23 37","l8"}
  r=t{"x","y"}="x\\2dy"
  !CheckEq{r,-1,"l9"}
  zs$=Str$(Instr(a$+b$,"\\44"))+" "+Left$(b$+"\\45",Len(a$+"\\46"))
  !CheckStrEq{zs$,"4 cDE","l10"}
!EndTestCase
; latin1_string: Latin-1 characters in strings and comments of a text
; source: the loader stores each byte from $80 of a string or a comment as
; a literal character, the umlauts too; the compiled string holds the
; bytes AmiBlitz3 gives it, a character literal the byte
!TestCase{latin1_string}
  a$ = "Größe" ; Größe éè in a comment
  !CheckEq{Len(a$),5,"l1"}
  zs$=Mid$(a$,1,1)
  !CheckEq{Asc(zs$),71,"l2"}
  zs$=Mid$(a$,2,1)
  !CheckEq{Asc(zs$),114,"l3"}
  zs$=Mid$(a$,3,1)
  !CheckEq{Asc(zs$),246,"l4"}
  zs$=Mid$(a$,4,1)
  !CheckEq{Asc(zs$),223,"l5"}
  zs$=Mid$(a$,5,1)
  !CheckEq{Asc(zs$),101,"l6"}
  b$ = "äÄýþÿ" : zs$=Right$(b$,1)
  !CheckStrEq{Str$(Asc(b$))+" "+Str$(Asc(zs$)),"228 255","l7"}
  ; the literal assigned first: AmiBlitz3 refuses a character from $80 in
  ; a literal inside a macro argument ("Syntax error!")
  le.l = @"é"
  !CheckStrEq{Hex$(le),"000000E9","l8"}
  lc.l = @"aé"
  !CheckStrEq{Hex$(lc),"000061E9","l9"}
!EndTestCase
