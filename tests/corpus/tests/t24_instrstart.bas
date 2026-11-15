; t24_instrstart - stringfunclib: Instr with a start position below 1 (#287)
; Released code searches from the first character; runtime error checking
; stops at the first call with "Instr Startpos MUST be greater than zero".
a$="Hello, World!"
NPrint "instr ",Instr(a$,"H")," ",Instr(a$,"o",1)
NPrint "start 0 ",Instr(a$,"",0)," ",Instr(a$,"World",0)
NPrint "start -5 ",Instr(a$,"",-5)," ",Instr(a$,"o",-5)
NPrint "done"
End
