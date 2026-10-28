; vallib-val-wortlaenge (#303) - Val counts the characters of the string
; with SUBQ #1,d2:BMI on the low word of the length: a string of 32769
; characters (word -32767) gives 0, whatever it holds.  The string buffer
; is raised to 70000 bytes (tedstub strbuf=) so that the string fits.
; Correct: Val of "5" followed by 32768 spaces gives 5.
; @configs clib
; @pass jobline=strbuf=70000
a$="5"+String$(" ",32768)
NPrint Len(a$)
NPrint Val(a$)
End
