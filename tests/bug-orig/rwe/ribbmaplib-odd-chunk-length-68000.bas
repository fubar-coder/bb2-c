; ribbmaplib-odd-chunk-length-68000 (#552) - the chunk search steps by
; 8 + length, without the pad byte IFF puts behind a chunk of odd length:
; the id of the next chunk is read one byte too early, at an odd address,
; as a long.  On a 68000 that is an address error (the session ends in a
; timeout); ribbmaplib-odd-chunk-length shows the 68020 side.  The
; machine is the A600 (68000).  The scenario is built in memory: FORM
; SCEN, an ANNO chunk of length 7 with its pad byte, then LEVL (2 bytes,
; level 5).
; Correct: BBScenarioLevel finds LEVL behind the padded chunk: 5.
; @fixed #552
; @rwe ribbmaplib
; @machine a600
; @pass timeout=30
DEFTYPE .l
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,30 : Poke.l m+8,$5343454E
Poke.l m+12,$414E4E4F : Poke.l m+16,7
Poke.l m+20,$42756720 : Poke.w m+24,$4F72 : Poke.b m+26,$67 : Poke.b m+27,0
Poke.l m+28,$4C45564C : Poke.l m+32,2 : Poke.w m+36,5
NPrint "before"
NPrint SetBBScenarioLoc(m)
NPrint BBScenarioLevel
FreeMem_ m,64
End
