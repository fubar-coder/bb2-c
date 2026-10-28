; t03_str - strings
a$="Hello" : b$="World"
c$=a$+", "+b$+"!"
NPrint c$," len ",Len(c$)
NPrint Left$(c$,5),"|",Right$(c$,6),"|",Mid$(c$,8,5),"|"
NPrint UCase$(c$),"|",LCase$(c$)
NPrint "instr ",Instr(c$,"World")," ",Instr(c$,"x")," ",Instr(c$,"l",5)
NPrint "chr ",Chr$(65),Chr$(66)," asc ",Asc("a")
NPrint "str ",Str$(-123),"|",Str$(4.5),"| val ",Val("42")+1
NPrint "hex ",Hex$(255)," bin ",Bin$(5)
NPrint "string ",String$("ab",3)
NPrint "replace ",Replace$(c$,"l","L")
NPrint "cmp ",a$<b$," ",a$=a$," ",a$>b$," ",a$<>b$
NPrint "lset [",LSet$("ab",5),"] rset [",RSet$("ab",5),"] centre [",Centre$("ab",6),"]"
d$="" : For i=1 To 5 : d$=d$+Str$(i) : Next
NPrint "built ",d$
MaxLen e$=10 : e$="0123456789"
NPrint "maxlen ",e$," ",Len(e$)
Dim s$(3) : s$(0)="zero" : s$(1)="one" : s$(2)="two" : s$(3)=s$(1)+s$(2)
For i=0 To 3 : NPrint i," ",s$(i) : Next
End
