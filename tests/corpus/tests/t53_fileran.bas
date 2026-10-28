; t53_fileran - fileiolib random access: Fields (all types), Put/Get in any
; order, record at the end of the file, reopening, Fields adding up
f$="RAM:t53.dat"
KillFile f$
MaxLen n$=10
DEFTYPE .l
NPrint "open ",OpenFile(0,f$)
Fields 0,a.l,b.w,c.b,q.q,x.f,n$
For i=0 To 4
  a=i*100000+7 : b=-i-1 : c=i*30 : q=i/4 : x=i*1.5 : n$="rec"+Str$(i)
  Put 0,i
Next
NPrint "lof ",Lof(0)," loc ",Loc(0)
For i=4 To 0 Step -1
  Get 0,i : NPrint i," ",a," ",b," ",c," ",q," ",x," [",n$,"]"
Next
a=-1 : b=2 : c=3 : q=0.5 : x=-1 : n$="new two"
Put 0,2
a=0 : n$=""
Get 0,3 : NPrint "3 ",a," [",n$,"]"
Get 0,2 : NPrint "2 ",a," ",b," ",c," ",q," ",x," [",n$,"]"
Get 0,5 : NPrint "at end ",a," [",n$,"] loc ",Loc(0)," eof ",Eof(0)
a=99 : Put 0,5
NPrint "appended lof ",Lof(0)," loc ",Loc(0)
CloseFile 0
NPrint "read ",ReadFile(1,f$)," lof ",Lof(1)
Fields 1,a,b
Get 1,0 : NPrint "first ",a," ",b
Fields 1,c
c=0 : Get 1,0 : NPrint "added ",a," ",b," ",c," loc ",Loc(1)
CloseFile 1
KillFile f$
End
