; compiler-amigalib-a4a5 (#958) - an Amiga library call pops its
; parameters into their registers before it loads the library base
; (move.l off(a5),a6) and saves neither a4 nor a5: a parameter in a5 makes
; a6 garbage (the jsr jumps anywhere), and after the call a4/a5 hold the
; parameters instead of the bases of the local/global variables.
; Here, in a procedure, workbench AddAppIconA_ (taglist in a4; its other
; parameters are global, SHARED) and the local l read after it; then
; graphics LockLayerRom_ and UnlockLayerRom_ (statements) and
; AttemptLockLayerRom_ (function), the layer in a5, on a layer of
; layers.library on a bitmap of its own, each followed by a read of the
; global g.  (exec Supervisor_, also a5, cannot be compiled at all: its
; token is also the token of doslib's finit, #960.)
; Correct: the program prints local 7, global 42 four times and locked -1.
DEFTYPE .l
Statement wb{}
  SHARED d,p,r,t$,tg
  l=7
  r=AddAppIconA_(0,0,&t$,p,0,d,tg)
  If l=7 Then NPrint "local ",l Else NPrint "local wrong"
End Statement
g=42
t$="bug-orig" : tg=?tags
d=GetDefDiskObject_(3)
p=CreateMsgPort_
wb{}
If r Then RemoveAppIcon_ r
DeleteMsgPort_ p
FreeDiskObject_ d
bm=AllocBitMap_(64,16,1,0,0)
li=NewLayerInfo_
ly=CreateUpfrontLayer_(li,bm,0,0,63,15,1,0)
LockLayerRom_ ly
If g=42 Then NPrint "global ",g Else NPrint "global wrong"
UnlockLayerRom_ ly
If g=42 Then NPrint "global ",g Else NPrint "global wrong"
x=AttemptLockLayerRom_(ly)
If g=42 Then NPrint "global ",g Else NPrint "global wrong"
UnlockLayerRom_ ly
If g=42 Then NPrint "global ",g Else NPrint "global wrong"
NPrint "locked ",x<>0
DeleteLayer_ 0,ly
DisposeLayerInfo_ li
FreeBitMap_ bm
End

tags:
  Dc.l 0
