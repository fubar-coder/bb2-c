; rilinkedlistlib-no-current-check (#733) - the commands take the
; current node of the list from their table without checking it for 0:
; SetListItem on list 1, which was never initialised, writes the pointer
; to its new copy into the data field of the "node" at address 0, i.e.
; to address 10.  Made safe: the long at 10 (part of the exception
; vectors) is read before and after the call with interrupts disabled
; and restored at once.
; Correct: SetListItem without a current node returns 0 and writes
; nothing.
; @rwe rilinkedlistlib riamosfunclib
NEWTYPE .t
  a.l
End NEWTYPE
DEFTYPE .t v
v\a=1
Disable_
o.l=Peek.l(10)
r.l=SetListItem(1,v)
n.l=Peek.l(10)
Poke.l 10,o
Enable_
If n=o Then NPrint "address 10 kept" Else NPrint "address 10 written"
NPrint r
End
