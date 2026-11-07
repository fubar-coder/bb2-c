; compiler-libsnode-name (#28) - makeblitzlibs keeps the name of every
; directory in blitz2:userlibs in a 38 byte node (name: 32 bytes) and
; copies the name without a length check: a directory name of 64
; characters (the UAE directory volume allows it) writes 33 bytes behind
; the node.  What lies behind the node differs from session to session
; (free memory, another allocation): the overrun alone crashed or hung
; some sessions and did nothing visible in others.  inject pad puts 64
; zeroed bytes behind every allocation of blitz2, so the overrun stays
; inside them and is reported at FreeMem (frlibslist at the end).
; Correct: the long name does no harm (cut or skipped) - the program
; compiles and runs, nothing is written behind an allocation.
; @fixed #28
; @pass timeout=45
; @pass wrap=@INJECT@|pad
; @pass dir=blitz2/userlibs/dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd
NPrint "hello"
End
