; compiler-amigalib-a4a5.test/child.bas: the program of the test, started
; by the case
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
