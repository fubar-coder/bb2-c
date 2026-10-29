; arexxlib-createargstring-len (#57) - CreateArgString allocates
; length+9 bytes but copies with DBF on the low word of the length: of a
; string of 70000 characters only 70000 mod 65536 = 4464 are copied, the
; rest of the (cleared) block stays 0. The string buffer is raised to
; 150000 bytes (tedstub strbuf=) so that the string fits.
; Correct: all 70000 characters copied, then the 0.
; @configs clib
; @pass jobline=strbuf=150000
s$=String$("a",70000)
a.l=CreateArgString(s$)
NPrint Peek.b(a+4463)," ",Peek.b(a+4464)," ",Peek.b(a+69999)," ",Peek.b(a+70000)
End
