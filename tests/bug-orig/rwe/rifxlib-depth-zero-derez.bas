; rifxlib-depth-zero-derez (#947) - Derez takes the depth of the source
; bitmap without a check (MOVE.b into sbitdepth, then SUBQ.w #1 for the
; DBRA loops): a source of depth 0 makes the word $FFFF, and the plane
; loops run 65536 times - the first one stores 65536 pairs of plane
; pointers over the library's 16-entry arrays and the 256 KB behind them
; (in the original its own code), the screen copy then copies through
; them.  The depth byte of bitmap 0 is set to 0 after a Derez of depth 1.
; The original has no error check area.  Here with runtime error
; checking.
; Correct: the Derez of depth 1 works; the error check refuses the
; source of depth 0: "Illegal BitMap Depth", nothing is written.
; @rwe rifxlib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,320,16,1
BitMap 1,320,16,1
Derez 0,1,0,0,1,8
NPrint "depth 1 done"
Poke.b Addr BitMap(0)+5,0
NPrint "before"
Derez 0,1,0,0,1,8
NPrint "after"
End
