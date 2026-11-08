; compiler-copyconst-limit (#997) - the longest operand texts that fit.
; copyconst copies the text of an assembler operand behind a flag word
; into namebuff (source operand, Dc) or namebuff2 (destination); namebuff2
; follows namebuff, 8 bytes follow namebuff2, then optstuff.
; A source operand's text stays in namebuff until the instruction is
; assembled; a destination operand overwrites namebuff2.  What fits, each
; text from a macro (kept): 125 characters in the source operand with a
; destination behind it, 261 in the source operand with a register as
; destination and in Dc (to the end of the 8 bytes), 133 in the
; destination operand (likewise); and 125 with a local label in Dc
; (copyconst puts the 59 characters of the last global label in front).
; Correct: the program compiles and stores 63, 131, 7, 131 and the
; address of the local label (difference 0).
; @fixed #997
Macro s
move.l #`1+`1+`2,dats
End Macro
Macro r
move.l #`1+`1+`1+`1+`2,d0
move.l d0,datr
End Macro
Macro d
move.l #7,dests+`1+`1
End Macro
Macro c
Dc.l `1+`1+`1+`1+`2
End Macro
Goto skip
.dats
Dc.l 0
.datr
Dc.l 0
.dests
Dc.l 0
.datc
!c{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1,1+1+1+1+1+1+1+1+1+1+1}
.skip
!s{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1,1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1}
!r{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1,1+1+1+1+1+1+1+1+1+1+1}
!d{0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0}
NPrint Peek.l(?dats)
NPrint Peek.l(?datr)
NPrint Peek.l(?dests)
NPrint Peek.l(?datc)
a.l=?ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
NPrint Peek.l(a)-a
End
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc:
'l
Dc.l 'l+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0+0
