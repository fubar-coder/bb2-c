; t166_sliceerr - blitzcoplib: a Slice on a slice in use is a runtime
; error (check routine slicechk, only with runtime error checking).
Slice 0,44,2
NPrint "slice ",Peek.w(Addr Slice(0))
Slice 0,100,2
NPrint "not reached"
End
