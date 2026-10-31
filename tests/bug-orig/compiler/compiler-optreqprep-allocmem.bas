; compiler-optreqprep-allocmem (#8) - optreqprep copies the arrow images
; of the options requester into chip memory from AllocMem without checking
; the result: when the allocation fails (inject: the first AllocMem of 64
; bytes chip memory), the 64 bytes go to address 0 (ExecBase pointer,
; exception vectors) and optreqdone frees address 0 at the end.
; Correct: blitz2 gives up at once (like the other failed allocations at
; start), compiles nothing and the system goes on.
; @pass wrap=@INJECT@|allocfail=64-64/chip/1
; @pass timeout=30
NPrint "hello"
End
