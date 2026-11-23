; arexxlib-createargstring-len (#57) - CreateArgString allocates
; length+9 bytes but copies with DBF on the low word of the length, and
; ra_Length is the low word too: of a string of 70000 characters only
; 70000 mod 65536 = 4464 are copied (ra_Length 4464), the rest of the
; (cleared) block stays 0. The string buffer is raised to 150000 bytes
; (tedstub strbuf=) so that the string fits.
; Correct: an ArgString holds at most 65535 characters (ra_Length is a
; word): the string is cut to 65535 characters, all of them copied, then
; the 0, in a block of 65535+9 bytes.
DEFTYPE .l

!TestCase{string_70000}
  s$=String$("a",70000)
  a.l=CreateArgString(s$)
  !CheckEq{Peek.l(a-8) AND $7FFFFFFF,65544,"block_size"}
  !CheckEq{Peek.w(a-4) AND $FFFF,65535,"ra_length"}
  !CheckEq{Peek.b(a+4463),97,"char_4463"}
  !CheckEq{Peek.b(a+4464),97,"char_4464"}
  !CheckEq{Peek.b(a+65534),97,"char_65534"}
  !CheckEq{Peek.b(a+65535),0,"terminator"}
!EndTestCase
