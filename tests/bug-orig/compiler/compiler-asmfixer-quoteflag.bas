; compiler-asmfixer-quoteflag (#343) - asmfixer evaluates the asm operands at
; the end of the pass, last line first (insasm prepends), and never resets
; quoteflag. getelement2 saves lasta6 with a long move, so also quoteflag,
; after getchar3 has read the opening quote of a character constant; eval5
; restores that long when it folds a constant binary operation. After
; #">"*8 quoteflag stays set: in the earlier #" " the quotes toggle it off
; and on again, getchar2 skips the space between them, the constant is
; empty and the immediate 0. Found with the RWE LES debug library
; (ExtractLabel: CMPI.b #" ",d2 is 0 in RILESDEBUGLIB, a later line has
; MOVE.w #">"*8,d0). Correct: the program prints 00000020.
MOVEQ #-1,d1
MOVE.l #" ",d1
PutReg d1,r.l
NPrint Hex$(r)
MOVE.w #">"*8,d1
End
