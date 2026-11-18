; rilinkedlistlib-addfirst-uncleared (#726) - AddFirstListItem allocates
; the new node without MEMF_CLEAR and writes only its next field: prev,
; pad, data and size keep what the memory held.  The program makes such
; memory likely (with multitasking off it allocates 200 blocks of the
; node's size, 32 bytes with memlib's header, fills them with $FF and
; frees them, so the node's allocation is one of them) and looks at the
; fields of the new node.
; Correct: prev, pad, data and size of the new node are 0.
; @fixed #726
; @rwe rilinkedlistlib riamosfunclib
NEWTYPE .t
  a.l
End NEWTYPE
DEFTYPE .t v
v\a=1
DEFTYPE .l
Dim m(199)
r=InitList(0,v)
Forbid_
For i=0 To 199
  m(i)=AllocMem_(32,0)
  If m(i)
    For j=0 To 28 Step 4
      Poke.l m(i)+j,-1
    Next
  EndIf
Next
For i=0 To 199
  If m(i) Then FreeMem_ m(i),32
Next
r=AddFirstListItem(0)
x=AddrListItem(0)
Permit_
c=0
If Peek.l(x+4)<>0 Then c=1
If Peek.w(x+8)<>0 Then c=1
If Peek.l(x+10)<>0 Then c=1
If Peek.l(x+14)<>0 Then c=1
If c Then NPrint "fields not cleared" Else NPrint "fields cleared"
End
