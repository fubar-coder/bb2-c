; arexxlib-getstring-a3 (#52) - GetRexxCommand and GetResultString copy
; the ArgString with its terminating 0 (ra_Length+1 bytes) and leave a3
; behind the 0 instead of on it.  In a string expression the next part
; is then written one byte too far: the result has the right length but
; contains the 0, and the last character of the next part is lost.
; Correct: GetRexxCommand(r,1)+"x" with "one" is "onex", GetResultString
; +"x" with "two" is "twox" (the length 4, the 4th character "x" = 120).
; @fixed #52
; @configs clib
rp.l=CreateMsgPort("bb2rexxa3")
r.l=CreateRexxMsg(rp,"bb","BB2HOST")
Dim m.l(15) : b.l=&m(0)
Poke.s b+20,"one"
Poke.w b,0 : Poke.l b+2,b+20 : Poke.l b+6,0
FillRexxMsg r,b
s$=GetRexxCommand(r,1)+"x"
NPrint Len(s$)," ",Asc(Mid$(s$,4))
Poke.l r+36,CreateArgString("two")
s$=GetResultString(r)+"x"
NPrint Len(s$)," ",Asc(Mid$(s$,4))
ClearRexxMsg r
DeleteRexxMsg r
DeleteMsgPort rp
End
