; compiler-macro-uninit-free-noend (#965) - the other error inside a Macro
; definition of compiler-macro-uninit-free: the source ends before End
; Macro ("Macro without End Macro"). domacro has added the node of the
; macro small with addhere (AllocMem without MEMF_CLEAR) and set only its
; use count; freemacs frees its text with the pointer and length the
; memory held. inject fill=a5 makes that memory the same in every run
; (text $a5a5a5a5, length $a5a5): exec's FreeMem raises the alert
; AN_BadFreeAddr (0100 000F), which inject alertlog records.
; Correct: "Macro without End Macro", no alert, the compiler frees what it
; allocated.
; @fixed #965
; @pass wrap=@INJECT@|fill=a5 alertlog track
NPrint "hello"
Macro small
a=1
NPrint a
