; compiler-libsnode-name (#28) - makeblitzlibs keeps the name of every
; directory in blitz2:userlibs in a 38 byte node (name: 32 bytes) and
; copies the name without a length check: a directory name of 100
; characters (the UAE directory volume allows it) writes 69 bytes behind
; the node.
; Correct: the long name does no harm (cut or skipped) - the program
; compiles and runs.
; @pass timeout=45
; @pass dir=blitz2/userlibs/dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd
NPrint "hello"
End
