; compiler-newtype-forward-transitive (#974) - .tb contains the forward
; type .tc by value, .ta contains .tb by value: .ta has to be laid out again
; after .tb, when .tc is defined.
; Correct: .tc 12 bytes, .tb 16 (k at 0, x at 2, z at 14), .ta 20 (m at 0,
; q at 16), an array field of .tb [2] in .te takes 32 bytes.
; @fixed #974
NEWTYPE .pp
  *p.tc
End NEWTYPE
NEWTYPE .tb
  k.w
  x.tc
  z.w
End NEWTYPE
NEWTYPE .ta
  m.tb
  q.l
End NEWTYPE
NEWTYPE .te
  f.tb[2]
  g.w
End NEWTYPE
NEWTYPE .tc
  r.l
  s.l
  t.l
End NEWTYPE
DEFTYPE .ta v
DEFTYPE .te w
v\m\x\t=7
v\m\z=8
v\q=9
w\f[1]\z=4
w\g=6
NPrint "c ",SizeOf .tc," b ",SizeOf .tb," z ",SizeOf .tb\z
NPrint "a ",SizeOf .ta," q ",SizeOf .ta\q
NPrint "e ",SizeOf .te," g ",SizeOf .te\g
NPrint v\m\x\t," ",v\m\z," ",v\q," ",w\f[1]\z," ",w\g
End
