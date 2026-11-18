; rilinkedlistlib-dellistitem-first (#728) - DelListItem of the first node
; should clear the prev of the node behind it, the new first node; it
; clears offset 0 of that node, its next (the v0.9 layout had prev
; there): the list ends at the new first node.  The program builds A B C,
; deletes A and goes forward from B.  Made safe: A's data pointer is
; cleared first (the copy stays allocated until the end), so that the
; DelListItem bug of #729 frees nothing.
; Correct: NextListItem from B reaches C.
; @fixed #728
; @rwe rilinkedlistlib riamosfunclib
NEWTYPE .t
  a.l
End NEWTYPE
DEFTYPE .t v
v\a=1
r.l=InitList(0,v)
r=AddLastListItem(0)
r=AddLastListItem(0)
c.l=AddrListItem(0)
r=FirstListItem(0)
Poke.l AddrListItem(0)+10,0
r=DelListItem(0)
r=NextListItem(0)
If AddrListItem(0)=c Then NPrint "C reached" Else NPrint "list cut"
End
