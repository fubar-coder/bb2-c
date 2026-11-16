; ciatrklib-init-length-vs-address (#438) - Protracker's mt_init clears
; the first long of every sample: a sample without a loop (repeat length
; 1) repeats its first two bytes after it has been played.  The library
; changed it so that it does not write behind the module, but compares
; the slot's module LENGTH with the sample's address (CMP.l a2,a4): a
; length is far smaller than any sample address, so the long is never
; cleared, and every sample without a loop ends in a loop of its first
; two bytes (a buzz or a DC level).  The module (generated) has one
; sample of 4 words without a loop, $11223344 $55667788; it is loaded and
; started (ChangeTrackerVBlank: nothing plays) and stopped.
; Correct: the sample's first long is 0, the rest stays.
; @fixed #438
; @rwe ciatrklib
; @pass file=i.mod|@TESTDATA@/ciatrklib-init-length-vs-address.mod
DEFTYPE .l
r=LoadTrackerModule(0,"CORPUS:i.mod")
NPrint "loaded ",r
If r=0 Then End
ChangeTrackerVBlank
r=StartTracker(0)
StopTracker
a=GetTrackerLocation(0)+2108
NPrint "first long ",Hex$(Peek.l(a))
NPrint "second long ",Hex$(Peek.l(a+4))
FreeTrackerModule 0
End
