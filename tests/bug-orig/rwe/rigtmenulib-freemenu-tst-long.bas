; rigtmenulib-freemenu-tst-long (#691) - GTFreeMenu declares Item and
; Subitem as words but decides with TST.l whether they are given: a
; negative upper word of d3 counts as "no sub item" and GTFreeMenu frees
; the whole item instead (a negative upper word of d2 frees the whole
; title, as an item: not done here, it crashes).  Here the upper word of
; d3 is set negative (GetReg) right before the call with a word
; variable; the program prints the lists (title, items, sub items) from
; the object (the titles at +4, a text at +4, a title's items at +8, an
; item's sub items at +18).
; Correct: the upper word does not matter: sub item S1 goes, A and S0
; stay.
; @fixed #691
; @rwe rigtmenulib
DEFTYPE .l
GTMenuTitle 0,0,"Project"
GTMenuItem 0,0,0,0,"A"
GTMenuItem 0,0,0,1,"B"
GTSubItem 0,0,0,0,0,"S0"
GTSubItem 0,0,0,0,1,"S1"
n.w=1
GetReg d3,$80000000
GTFreeMenu 0,0,0,n
t=Peek.l(Addr GTMenuList(0)+4)
NPrint Peek$(Peek.l(t+4))
i=Peek.l(t+8)
While i
  NPrint " ",Peek$(Peek.l(i+4))
  s=Peek.l(i+18)
  While s
    NPrint "  ",Peek$(Peek.l(s+4))
    s=Peek.l(s)
  Wend
  i=Peek.l(i)
Wend
End
