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
; @fixed #833
; @rwe ristcdecrunchlib ristonecrackerlib
; @pass file=c1.pack410|@ANCIENT@/testing/test_files/test_C1.pack410
; @pass file=c1.pack410_0|@ANCIENT@/testing/test_files/test_C1.pack410_0
; @pass file=c1.raw|@ANCIENT@/testing/test_files/test_C1.raw
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
pad=65536 : g=66000 : un=152089
fb=AllocMem_(pad+77712+16,1)
wb=AllocMem_(pad+77712+16,1)
r=AllocMem_(un+16,1)
db=AllocMem_(g+un+200+70000,1)
ib=AllocMem_(80+un+16,1)
If fb=0 OR wb=0 OR r=0 OR db=0 OR ib=0 Then NPrint "no memory" : End
f=fb+pad : w=wb+pad : d=db+g
fill{fb,pad,$11111111} : fill{wb,pad,$11111111}
If ReadFile(0,"CORPUS:c1.raw") Then ReadMem 0,r,un : CloseFile 0
For e=0 To 1
  n$="c1.pack410" : l=64892
  If e=1 Then n$="c1.pack410_0" : l=77712
  If ReadFile(0,"CORPUS:"+n$) Then ReadMem 0,f,l : CloseFile 0
  fill{db,g+un+200+70000,$5A5A5A5A}
  c=STCDecrunch(f,d)
  NPrint n$," to dest: ",c," equal ",same{d,r,un}," below kept ",kept{db,g}," above kept ",kept{d+un,200}
  CopyMem_ f,ib,l
  c=STCDecrunch(ib)
  NPrint n$," in place: ",c=ib+80," equal ",same{ib+80,r,un}
  fill{ib,80+un,0}
  c=STCLoad("CORPUS:"+n$,ib)
  NPrint n$," STCLoad: ",c=ib+80," equal ",same{ib+80,r,un}
Next
If ReadFile(0,"CORPUS:c1.pack410") Then ReadMem 0,f,64892 : CloseFile 0
For v=0 To 4
  CopyMem_ f,w,64892
  Select v
    Case 0 : Poke.w w+12+Peek.l(w+12),17 : v$="efficiency 17"
    Case 1 : Poke.l w+12,Peek.l(w+12)+1 : v$="odd packed length"
    Case 2 : Poke.l w+8,un+100 : v$="unpacked length +100"
    Case 3 : Poke.l w+8,un-100 : v$="unpacked length -100"
    Case 4
      b=Peek.b(w+20)&255
      If b&1 Then b-1 Else b+1
      Poke.b w+20,b : v$="a bit flipped near the end"
  End Select
  fill{db,g+un+200+70000,$5A5A5A5A}
  c=STCDecrunch(w,d)
  If v<2
    NPrint v$,": ",c," below kept ",kept{db,g}
  Else
    NPrint v$,": below kept ",kept{db,g}
  EndIf
Next
End
