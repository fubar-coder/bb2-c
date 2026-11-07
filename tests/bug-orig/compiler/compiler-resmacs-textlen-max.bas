; compiler-resmacs-textlen-max (#995) - writing a resident, resmacs
; rounds the length of a macro's text up to even in a word: a text of
; exactly 65535 bytes gets the length 0 and is not written, but the
; macro's text pointer is set and relocated, and points at what follows
; the macro in the resident. The resident's macro big has 862 lines of
; 76 bytes and a last line of 23 bytes that sets b=7: 65535 bytes of
; text; the constant #c=5 follows it. The resident is compiled with the
; Macro Buffer of the defaults (100000 bytes). Latent in the original:
; its macrobuff always has 8192 bytes (#336), so it cannot make the
; resident ("Macro too Big") - the verdict against it is DIFFERS.
; Correct: the program finds the constant and expands the whole macro; it
; prints 7 and 5.
; @pass file=r.bb2|@TESTDATA@/compiler-resmacs-textlen-max.res
; @pass jobline=macrobuf=100000
; @pass jobline=resident=r|CORPUS:r.bb2|CORPUS:out/r.res|defaults
; @pass timeout=60
b.l=0
!big
NPrint b
NPrint #c
End
