; arexxlib-createargstring-len (#57) - CreateArgString allocates
; length+9 bytes but copies with DBF on the low word of the length, and
; ra_Length is the low word too: of a string of 70000 characters only
; 70000 mod 65536 = 4464 are copied (ra_Length 4464), the rest of the
; (cleared) block stays 0. The string buffer is raised to 150000 bytes
; (tedstub strbuf=) so that the string fits.
; Correct: an ArgString holds at most 65535 characters (ra_Length is a
; word): the string is cut to 65535 characters, all of them copied, then
; the 0, in a block of 65535+9 bytes.
; @fixed #57
; @configs clib
; @pass jobline=strbuf=150000
s$=String$("a",70000)
a.l=CreateArgString(s$)
NPrint Peek.l(a-8) AND $7FFFFFFF," ",Peek.w(a-4) AND $FFFF
NPrint Peek.b(a+4463)," ",Peek.b(a+4464)," ",Peek.b(a+65534)," ",Peek.b(a+65535)
End
