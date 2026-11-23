; compiler-newtype-forward-transitive (#974) - .tb contains the forward
; type .tc by value, .ta contains .tb by value: .ta has to be laid out again
; after .tb, when .tc is defined.
; Correct: .tc 12 bytes, .tb 16 (k at 0, x at 2, z at 14), .ta 20 (m at 0,
; q at 16), an array field of .tb [2] in .te takes 32 bytes.
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

!TestCase{forward_transitive}
  v\m\x\t=7
  v\m\z=8
  v\q=9
  w\f[1]\z=4
  w\g=6
  !CheckEq{SizeOf .tc,12,"size_tc"}
  !CheckEq{SizeOf .tb,16,"size_tb"}
  !CheckEq{SizeOf .tb\z,14,"offset_z"}
  !CheckEq{SizeOf .ta,20,"size_ta"}
  !CheckEq{SizeOf .ta\q,16,"offset_q"}
  !CheckEq{SizeOf .te,34,"size_te"}
  !CheckEq{SizeOf .te\g,32,"offset_g"}
  !CheckEq{v\m\x\t,7,"x_t"}
  !CheckEq{v\m\z,8,"m_z"}
  !CheckEq{v\q,9,"q"}
  !CheckEq{w\f[1]\z,4,"f1_z"}
  !CheckEq{w\g,6,"g"}
!EndTestCase
