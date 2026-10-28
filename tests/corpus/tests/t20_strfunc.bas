; t20_strfunc - stringfunclib: every token, edge cases
a$="Hello, World!"
NPrint "len ",Len(a$)," ",Len("")," ",Len(a$+a$)
Print "left [",Left$(a$,5),"][",Left$(a$,0),"][",Left$(a$,-3),"][",Left$(a$,13),"]["
NPrint Left$(a$,99),"][",Left$("",4),"]"
Print "right [",Right$(a$,6),"][",Right$(a$,0),"][",Right$(a$,-1),"][",Right$(a$,13),"]["
NPrint Right$(a$,50),"][",Right$("",2),"]"
NPrint "mid3 [",Mid$(a$,8,5),"][",Mid$(a$,1,1),"][",Mid$(a$,0,3),"][",Mid$(a$,-5,3),"][",Mid$(a$,13,5),"]"
Print "mid3 [",Mid$(a$,14,1),"][",Mid$(a$,15,1),"][",Mid$(a$,3,0),"][",Mid$(a$,3,-2),"]["
NPrint Mid$(a$,10,100),"][",Mid$("",1,1),"]"
Print "mid2 [",Mid$(a$,8),"][",Mid$(a$,1),"][",Mid$(a$,0),"][",Mid$(a$,13),"][",Mid$(a$,14),"]["
NPrint Mid$(a$,20),"][",Mid$(a$,-2),"]"
Print "unleft [",UnLeft$(a$,3),"][",UnLeft$(a$,0),"][",UnLeft$(a$,13),"][",UnLeft$(a$,20),"]["
NPrint UnLeft$(a$,-2),"]"
Print "unright [",UnRight$(a$,3),"][",UnRight$(a$,0),"][",UnRight$(a$,13),"][",UnRight$(a$,20)
NPrint "][",UnRight$(a$,-2),"]"
NPrint "hex ",Hex$(0)," ",Hex$(255)," ",Hex$(-1)," ",Hex$($80000000)," ",Hex$($12345678)," ",Hex$($ABCDEF)
NPrint "bin ",Bin$(5)
NPrint "bin ",Bin$(-1)
NPrint "bin ",Bin$($80000001)
Print "chr [",Chr$(65),Chr$(66),"][",Chr$(256+67),"][",Chr$(-191),"] ",Len(Chr$(0))," "
NPrint Asc(Chr$(0))," ",Asc(Chr$(200))," ",Asc(Chr$(255))
NPrint "asc ",Asc("a")," ",Asc("")," ",Asc("Zebra")," ",Asc(Mid$(a$,5,1))
Print "string [",String$("ab",3),"][",String$("",5),"][",String$("x",1),"] "
NPrint Len(String$("abc",700))," ",Right$(String$("abcdefgh",500),3)
b$=String$("-=",10) : NPrint "string var [",b$,"] ",Len(b$)
Print "instr ",Instr(a$,"World")," ",Instr(a$,"x")," ",Instr(a$,"l",5)," ",Instr(a$,"l",11)," "
NPrint Instr(a$,"",1)," ",Instr("","")," ",Instr("","a")
Print "instr ",Instr(a$,"!",13)," ",Instr(a$,"!",14)," ",Instr(a$,"Hello, World!!")," "
NPrint Instr(a$,"H")," ",Instr(a$,"",0)," ",Instr(a$,"",-5)," ",Instr(a$,"o",100)
NPrint "instr case ",Instr(a$,"WORLD")," ",Instr("ABC","b")," ",Instr(a$,"hello")
CaseSense Off
Print "instr nocase ",Instr(a$,"WORLD")," ",Instr("ABC","b")," ",Instr(a$,"hello")," "
NPrint Instr("a[b{c","{")," ",Instr("a[b{c","[")," ",Instr("x@y`z","`")
Print "replace nocase [",Replace$(a$,"L","_"),"][",Replace$("aAbBaA","a","x"),"]["
NPrint Replace$(a$,"WORLD","There"),"]"
CaseSense On
NPrint "instr case ",Instr(a$,"WORLD")," ",Instr("ABC","b")," ",Instr(a$,"hello")
NPrint "replace [",Replace$(a$,"l","L"),"][",Replace$(a$,"World","There"),"][",Replace$(a$,"o",""),"]"
Print "replace [",Replace$("aaa","a","bb"),"][",Replace$("aaaa","aa","b"),"]["
NPrint Replace$(a$,"xyz","q"),"][",Replace$("","a","b"),"]"
Print "replace [",Replace$(a$,"!","?!"),"][",Replace$(a$,"Hello, World!","X"),"]["
NPrint Replace$("ab","abc","X"),"][",Replace$("aAbBaA","a","x"),"]"
c$=Replace$(String$("ab",20),"b","123") : NPrint "replace var [",c$,"] ",Len(c$)
Print "mki ",Len(Mki$(5))," ",Cvi(Mki$(1234))," ",Cvi(Mki$(-2))," ",Cvi(Mki$($12345))," "
NPrint Asc(Mki$($4142))," ",Asc(Right$(Mki$($4142),1))
Print "mkl ",Len(Mkl$(5))," ",Cvl(Mkl$($12345678))," ",Cvl(Mkl$(-1))," "
NPrint Asc(Mid$(Mkl$($41424344),3,1))," ",Hex$(Cvl(Mkl$($80000000)))
q.q=Cvq(Mkq$(1.5)) : r.q=Cvq(Mkq$(-3.25))
NPrint "mkq ",Len(Mkq$(1))," ",q," ",r," ",Hex$(Cvl(Mkq$(-3.25)))
NPrint "cv ",Cvi("AB")," ",Cvl("ABCD")," ",Hex$(Cvl("ABCDEF"))," ",Cvq("ABCD")," ",Cvi("ABCD")
Print "lset [",LSet$("ab",5),"][",LSet$("abcdef",3),"][",LSet$("ab",2),"][",LSet$("ab",0),"]["
NPrint LSet$("ab",-1),"][",LSet$("",3),"]"
Print "rset [",RSet$("ab",5),"][",RSet$("abcdef",3),"][",RSet$("ab",2),"][",RSet$("ab",0),"]["
NPrint RSet$("ab",-1),"][",RSet$("",3),"]"
Print "centre [",Centre$("abc",7),"][",Centre$("abc",6),"][",Centre$("ab",7),"]["
NPrint Centre$("ab",6),"][",Centre$("",4),"][",Centre$("abc",3),"]"
Print "centre [",Centre$("abcdefg",3),"][",Centre$("abcdefg",4),"][",Centre$("abcdef",3),"]["
NPrint Centre$("abcdef",4),"][",Centre$("ab",-1),"][",Centre$("a",2),"]"
d$="<"+LSet$("ab",4)+"|"+RSet$("cd",4)+"|"+Centre$("ef",5)+">" : NPrint "concat ",d$," ",Len(d$)
Print "case [",LCase$(a$),"][",UCase$(a$),"][",LCase$(""),"][",UCase$(""),"]["
NPrint UCase$("az@[`{AZ09"),"][",LCase$("az@[`{AZ09"),"]"
e$=Chr$(196)+Chr$(228)+"x" : f$=UCase$(e$) : g$=LCase$(e$)
NPrint "case8 ",Asc(f$)," ",Asc(Mid$(f$,2,1))," ",Asc(Mid$(f$,3,1))," ",Asc(g$)," ",Asc(Mid$(g$,2,1))
Print "striplead [",StripLead$("   abc  ",32),"][",StripLead$("xxxx",Asc("x")),"]["
NPrint StripLead$("",32),"][",StripLead$("abc",32),"][",StripLead$("aab",97+256),"]"
Print "striptrail [",StripTrail$("   abc  ",32),"][",StripTrail$("xxxx",Asc("x")),"]["
NPrint StripTrail$("",32),"][",StripTrail$("abc",32),"][",StripTrail$("abb",98+512),"]"
Print "nest [",UCase$(Left$(a$,5))+LCase$(Right$(a$,6)),"][",Mid$(Replace$(a$,"o","0"),5,4),"]["
NPrint Left$(Hex$(255),6),"]"
h$=StripTrail$(StripLead$("  pad me  ",32),32) : NPrint "nest2 [",h$,"] ",Len(h$)
For i=-1 To 4
  Print "loop ",i," [",Left$("abc",i),"][",Right$("abc",i),"][",Mid$("abc",i),"]"
  NPrint "[",Mid$("abc",i,2),"][",String$("ab",i+2),"][",LSet$("abc",i),"][",Centre$("abc",i),"]"
Next
NPrint "string0 [",String$("ab",0),"][",String$("ab",-2),"] ",Len(String$("q",0))
NPrint "replace empty [",Replace$(a$,"","x"),"]"
NPrint "end"
End
