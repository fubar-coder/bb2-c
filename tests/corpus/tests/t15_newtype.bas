; t15_newtype - NewType fields that point to the type being defined
; (*nxt.node: a pointer, 4 bytes; a field of it by value is an error,
; #973), a field of an earlier NewType by value, offsets with SizeOf
NEWTYPE .pos
  x.w
  y.w
End NEWTYPE
NEWTYPE .node
  num.l
  *nxt.node
  p.pos
  *prv.node
End NEWTYPE
NPrint "size ",SizeOf .node
o.l=SizeOf .node\nxt : NPrint "nxt at ",o
o=SizeOf .node\p : NPrint "p at ",o
o=SizeOf .node\prv : NPrint "prv at ",o
DEFTYPE .node a,b,c
a\num=1 : b\num=2 : c\num=3
a\nxt=&b : b\nxt=&c : c\nxt=0
c\prv=&b : b\prv=&a : a\prv=0
b\p\x=7,-7
*n.node=&a
While *n
  NPrint "num ",*n\num," p ",*n\p\x," ",*n\p\y
  *n=*n\nxt
Wend
NPrint "back ",c\prv\prv\num," ",a\nxt\nxt\prv\num
End
