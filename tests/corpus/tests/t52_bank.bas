; t52_bank - banklib: InitBank/FreeBank/Bank/BankLoc/BankSize,
; LoadBank/SaveBank round trip (RAM:), AllocMem/FreeMem
f$="RAM:t52.bank"
KillFile f$
NPrint "unused ",Bank(0)," ",BankLoc(0)," ",BankSize(0)
InitBank 0,100,65536
NPrint "init ",Bank(0)<>0," ",BankLoc(0)=Bank(0)," ",BankSize(0)," ",Peek.l(Bank(0))
For i=0 To 99 : Poke.b Bank(0)+i,i*3 : Next
SaveBank 0,f$
NPrint "saved ",Exists(f$)
InitBank 1,40,65536
LoadBank 1,f$
NPrint "load into smaller ",BankSize(1)," ",Peek.b(Bank(1)+1)," ",Peek.b(Bank(1)+99)
InitBank 2,200,0
For i=0 To 199 : Poke.b Bank(2)+i,7 : Next
LoadBank 2,f$
Print "load into larger ",BankSize(2)," ",Peek.b(Bank(2)+98)," ",Peek.b(Bank(2)+99)," ",Peek.b(Bank(2)+100)
NPrint " ",Peek.b(Bank(2)+199)
LoadBank 3,f$
NPrint "load into unused ",Bank(3)<>0," ",BankSize(3)," ",Peek.b(Bank(3)+50)
LoadBank 2,f$,2
NPrint "other memtype ",BankSize(2)," ",Peek.b(Bank(2)+4)
LoadBank 4,"RAM:t52.missing"
NPrint "missing ",Bank(4)," ",BankSize(4)
FreeBank 1
NPrint "freed ",Bank(1)," ",BankSize(1)
FreeBank 1
InitBank 3,0,0
NPrint "init 0 ",Bank(3)," ",BankSize(3)
SaveBank 3,"RAM:t52.none"
NPrint "save unused ",Exists("RAM:t52.none")
InitBank 0,16,65536
NPrint "reinit ",BankSize(0)," ",Peek.l(Bank(0))," ",Peek.l(Bank(0)+12)
a.l=AllocMem(64,65536)
NPrint "allocmem ",a<>0," ",Peek.l(a)," ",Peek.l(a+60)
Poke.l a,12345 : NPrint Peek.l(a)
FreeMem a,64
b.l=AllocMem(8,0) : NPrint "allocmem2 ",b<>0 : FreeMem b,8
KillFile f$
End
