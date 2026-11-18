; rigtmenulib-freemenu-subitem-minus-one (#1089) - the boundary of the
; fix, not a bug of the original: GTFreeMenu's error check refuses a
; negative item number, but a negative sub item number stays allowed
; (user decision W3 "Sub-Item -1 erlauben", as GTGetUserData's sub item
; -1): GTFreeMenu list#,menu,item,-1 removes the whole item, as in the
; original.  Here with runtime error checking: a valid sub item call
; (sub item 0 of item 0), then the sub item -1 of item 1; the program
; prints the items of the title (the titles at +4 of the object, a
; title's items at +8, an item's text at +4, its sub items at +18).
; Expected (what the original does): no error, item B is gone, A stays
; without its sub item.
; @rwe rigtmenulib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
GTMenuTitle 0,0,"Project"
GTMenuItem 0,0,0,0,"A"
GTMenuItem 0,0,0,1,"B"
GTSubItem 0,0,0,0,0,"S0"
NPrint "before"
GTFreeMenu 0,0,0,0
GTFreeMenu 0,0,1,-1
t=Peek.l(Addr GTMenuList(0)+4)
i=Peek.l(t+8)
While i
  NPrint " ",Peek$(Peek.l(i+4))," subs ",Peek.l(i+18)
  i=Peek.l(i)
Wend
NPrint "after"
End
