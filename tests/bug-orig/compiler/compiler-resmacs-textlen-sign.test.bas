; compiler-resmacs-textlen-sign (#995) - loading a resident, fixmacs steps
; past the last macro with its text length rounded up to even in a word
; and added sign-extended (add d1,a3): a last macro with a text of 32767
; bytes or more moves the load pointer backwards, and the types and
; constants are read from the wrong place. The resident's macro big has
; 526 lines of 76 bytes and a last line of 23 bytes that sets b=7: 39999
; bytes of text; the constant #c=5 follows it. The resident is compiled
; with the Macro Buffer of the defaults (100000 bytes). Latent in the
; original: its macrobuff always has 8192 bytes (#336), so it cannot
; make the resident ("Macro too Big").
; Correct: the program finds the constant and expands the macro; it
; sets b to 7 and #c is 5.
DEFTYPE .l

!TestCase{macro_text_sign}
  b=0
  !big
  !CheckEq{b,7,"macro_expanded"}
  !CheckEq{#c,5,"constant"}
!EndTestCase
