; ristcdecrunchlib-optidec-real-data (#833) - the S404 decruncher (also
; in ristonecrackerlib) on real data: ancient's test_C1.pack410 and
; test_C1.pack410_0 (152089 bytes of text packed by StoneCracker 4.10,
; efficiency 14 and 10) unpacked to another address, in place and with
; STCLoad must give test_C1.raw.  Then corrupt copies of test_C1.pack410
; made here, unpacked to a destination with 66000 guard bytes below it:
; efficiency 17, an odd packed length, the unpacked length 100 larger and
; 100 smaller, one bit flipped in a word read near the end.  The original
; checks nothing: such data write a match or literal run past the start
; of the destination into the memory below.  The files lie behind 64 KB
; of a fixed pattern, so the words a corrupt stream reads below them are
; the same in every run.
; Correct: the real data unpack to test_C1.raw; nothing of the corrupt
; data is written below the destination; efficiency 17 and the odd
; packed length are refused (0).
DEFTYPE .l
Statement fill{a.l,n.l,v.l}
  For k.l=a To a+n-4 Step 4 : Poke.l k,v : Next
End Statement
Function.l same{a.l,b.l,n.l}
  For k.l=0 To n-4 Step 4
    If Peek.l(a+k)<>Peek.l(b+k) Then Function Return 0
  Next
  For k=k To n-1
    If Peek.b(a+k)<>Peek.b(b+k) Then Function Return 0
  Next
  Function Return -1
End Function
Function.l kept{a.l,n.l}
  For k.l=a To a+n-4 Step 4
    If Peek.l(k)<>$5A5A5A5A Then Function Return 0
  Next
  Function Return -1
End Function

!TestCase{real_data}
  Dim vc.l(5),vk.l(5)
  pad=65536 : g=66000 : un=152089
  fb=AllocMem_(pad+77712+16,1)
  wb=AllocMem_(pad+77712+16,1)
  r=AllocMem_(un+16,1)
  db=AllocMem_(g+un+200+70000,1)
  ib=AllocMem_(80+un+16,1)
  !Require{fb<>0 AND wb<>0 AND r<>0 AND db<>0 AND ib<>0,"memory"}
  f=fb+pad : w=wb+pad : d=db+g
  fill{fb,pad,$11111111} : fill{wb,pad,$11111111}
  If ReadFile(0,"CORPUS:c1.raw") Then ReadMem 0,r,un : CloseFile 0
  ; the file c1.pack410 (64892 bytes), efficiency 14
  If ReadFile(0,"CORPUS:c1.pack410") Then ReadMem 0,f,64892 : CloseFile 0
  fill{db,g+un+200+70000,$5A5A5A5A}
  c=STCDecrunch(f,d)
  u1=(c=1) AND same{d,r,un}
  w1=kept{db,g}
  x1=kept{d+un,200}
  CopyMem_ f,ib,64892
  c=STCDecrunch(ib)
  y1=(c=ib+80) AND same{ib+80,r,un}
  fill{ib,80+un,0}
  c=STCLoad("CORPUS:c1.pack410",ib)
  z1=(c=ib+80) AND same{ib+80,r,un}
  ; the file c1.pack410_0 (77712 bytes), efficiency 10
  If ReadFile(0,"CORPUS:c1.pack410_0") Then ReadMem 0,f,77712 : CloseFile 0
  fill{db,g+un+200+70000,$5A5A5A5A}
  c=STCDecrunch(f,d)
  u2=(c=1) AND same{d,r,un}
  w2=kept{db,g}
  x2=kept{d+un,200}
  CopyMem_ f,ib,77712
  c=STCDecrunch(ib)
  y2=(c=ib+80) AND same{ib+80,r,un}
  fill{ib,80+un,0}
  c=STCLoad("CORPUS:c1.pack410_0",ib)
  z2=(c=ib+80) AND same{ib+80,r,un}
  ; the corrupt copies of c1.pack410
  If ReadFile(0,"CORPUS:c1.pack410") Then ReadMem 0,f,64892 : CloseFile 0
  For v=0 To 4
    CopyMem_ f,w,64892
    Select v
      Case 0 : Poke.w w+12+Peek.l(w+12),17
      Case 1 : Poke.l w+12,Peek.l(w+12)+1
      Case 2 : Poke.l w+8,un+100
      Case 3 : Poke.l w+8,un-100
      Case 4
        b=Peek.b(w+20)&255
        If b&1 Then b-1 Else b+1
        Poke.b w+20,b
    End Select
    fill{db,g+un+200+70000,$5A5A5A5A}
    vc(v)=STCDecrunch(w,d)
    vk(v)=kept{db,g}
  Next
  ef=vc(0) : eo=vc(1)
  k0=vk(0) : k1=vk(1) : k2=vk(2) : k3=vk(3) : k4=vk(4)
  !CheckEq{u1,-1,"pack410_to_dest"}
  !CheckEq{w1,-1,"pack410_below_kept"}
  !CheckEq{x1,-1,"pack410_above_kept"}
  !CheckEq{y1,-1,"pack410_in_place"}
  !CheckEq{z1,-1,"pack410_stcload"}
  !CheckEq{u2,-1,"pack410_0_to_dest"}
  !CheckEq{w2,-1,"pack410_0_below_kept"}
  !CheckEq{x2,-1,"pack410_0_above_kept"}
  !CheckEq{y2,-1,"pack410_0_in_place"}
  !CheckEq{z2,-1,"pack410_0_stcload"}
  !CheckEq{ef,0,"efficiency_17_refused"}
  !CheckEq{k0,-1,"efficiency_17_below_kept"}
  !CheckEq{eo,0,"odd_length_refused"}
  !CheckEq{k1,-1,"odd_length_below_kept"}
  !CheckEq{k2,-1,"length_plus_100_below_kept"}
  !CheckEq{k3,-1,"length_minus_100_below_kept"}
  !CheckEq{k4,-1,"bit_flip_below_kept"}
!EndTestCase
