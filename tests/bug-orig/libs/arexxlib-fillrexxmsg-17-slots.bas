; arexxlib-fillrexxmsg-17-slots (#49) - FillRexxMsg and ClearRexxMsg have
; no limit of the 16 argument slots rm_Args[0..15].  FillRexxMsg writes a
; 17th value of the FillStruct to rm_PassPort ($68); ClearRexxMsg with all
; 16 slots used goes on into rm_PassPort, rm_CommAddr and rm_FileExt,
; deletes what looks like an ArgString there (the host and extension
; ArgStrings of CreateRexxMsg) and clears the fields.
; The 16 integer values point to "x" with a length word of 7 in front, so
; that ClearRexxMsg does not take them for ArgStrings.
; Correct: rm_PassPort stays 0 after FillRexxMsg; ClearRexxMsg leaves
; rm_PassPort and rm_CommAddr as they were.
; @fixed #49
; @configs clib
Dim m.l(63)
b.l=&m(0)
For i=0 To 63 : m(i)=0 : Next
rp.l=CreateMsgPort("bb2fill")
r.l=CreateRexxMsg(rp,"bb","BB2HOST")
Poke.w b+196,7 : Poke.s b+200,"x" : Poke.s b+208,"y"
Poke.w b,$FFFF
For i=0 To 15 : Poke.l b+2+i*4,b+200 : Next
Poke.l b+66,b+208 : Poke.l b+70,0
FillRexxMsg r,b
NPrint "fill passport ",Peek.l(r+$68)=0," slot16 ",Peek.l(r+$64)=b+200
Poke.l r+$68,rp
ClearRexxMsg r
NPrint "clear passport ",Peek.l(r+$68)=rp," host ",Peek.l(r+$6C)<>0," slot16 ",Peek.l(r+$64)
Poke.l r+$68,0
DeleteRexxMsg r
DeleteMsgPort rp
End
