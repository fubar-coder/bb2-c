; rigtmenulib-freemenu-negative-item (#1089) - GTFreeMenu list#,menu,item
; treats a negative item number as "no item given" and frees the title
; through the item path: as an item of 30 bytes (the title node has 12),
; with the "shortcut" and the "sub items" read behind the node.  GTFreeMenu
; has no error check.  Here with runtime error checking and the item -1.
; Correct: the error check refuses the item number: "Illegal Menu Item
; number".
; @fixed #1089
; @rwe rigtmenulib
; @pass timeout=30
; @pass runtime_errors=on
GTMenuTitle 0,0,"Project"
GTMenuItem 0,0,0,0,"A"
NPrint "before"
GTFreeMenu 0,0,-1
NPrint "after"
End
