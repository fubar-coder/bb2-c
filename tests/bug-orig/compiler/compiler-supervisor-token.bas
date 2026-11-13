; compiler-supervisor-token (#960) - fixlibs numbers every entry of a
; user library from libnum*128+1, nullsubs included, and ends the
; library's token range behind its last entry.  doslib (252) has 127 calls
; and then its init and finit nullsubs, so its range runs to $FE81, which
; is exec's (253) first token, Supervisor_: findtoke finds that token in
; doslib (finit, no command) and the line is a Syntax Error.
; Here exec Supervisor_ runs a routine in supervisor mode that reads the
; status register (S bit 13) and returns with RTE.
; Correct: the program compiles and prints supervisor 8192.
; @fixed #960
DEFTYPE .l
r=Supervisor_(?sup)
NPrint "supervisor ",r
End

sup:
  MOVE.w sr,d0
  ANDI.l #$2000,d0
  RTE
