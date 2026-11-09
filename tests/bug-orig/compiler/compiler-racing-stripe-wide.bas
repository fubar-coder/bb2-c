; compiler-racing-stripe-wide (#980) - while compiling from the editor the
; compiler XORs its progress bar ("racing stripe") into plane 0 of TED's
; screen (comdata+36): the main bar in rows 0 and 9, the bars of include
; levels 1 to 3 in rows 1 to 3 and 8 to 6. The offset of row 9 is
; 9 * bytes per row (comdata+92), kept and added as a signed word: from
; 3641 bytes per row the compiler writes about 28 KB before the bitmap.
; Here tedstub gives a bitmap of 4000 bytes per row with 49152 guard bytes
; before and after it; the program has more lines than the bar has cells
; and three nested includes, so every bar moves. Job j000 compiles it
; without TED-C's progress call (comdata+98/102), job j001 with it.
; Correct: the guards are untouched in both jobs; without the call the
; bitmap stays empty, with it the compiler reports the cells the original
; XORs and nothing else: in each of the two passes every cell of the main
; bar (73 for the 640 pixel window) and one cell per include line (11, 7
; and 4 lines). The stub logs the calls per cell and the fullest picture
; of its bitmap (the second pass toggles the cells back).
; @fixed #980
; @pass file=src/inc1.bb2|@TESTDATA@/compiler-racing-stripe-wide.inc1
; @pass file=src/inc2.bb2|@TESTDATA@/compiler-racing-stripe-wide.inc2
; @pass file=src/inc3.bb2|@TESTDATA@/compiler-racing-stripe-wide.inc3
; @pass jobline=bitmap=4000|49152
; @pass jobline=progress=on
; @pass jobline=job=j001|CORPUS:src/compiler-racing-stripe-wide.bb2|CORPUS:out/stripe-j001
Include "inc1.bb2"
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
a=a+1
NPrint a+b+c+d
End
