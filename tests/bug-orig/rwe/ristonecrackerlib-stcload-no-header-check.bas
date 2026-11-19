; ristonecrackerlib-stcload-no-header-check (#836) - STCLoad reads the
; whole file and unpacks it in place without checking the "S404" id or
; that the file holds the packed data and the trailer its header names:
; any other file is "unpacked" with what its words say, and the result is
; the address of the data.  Three files in RAM:, each loaded to a buffer
; filled with $11: a valid S404 file (one literal byte "A", unpacked to
; buffer + 24), the same with the id "S403", and an S404 header whose
; packed length (1000) lies beyond the end of the 22-byte file.  The two
; bad ones unpack 0 bytes, so the original writes nothing.
; Correct: the valid file gives the address and "A"; the other two give 0
; (as documented: "If this command returns 0 then the file could not be
; loaded properly or was not a valid packed file").
; @fixed #836
; @rwe ristonecrackerlib
DEFTYPE .l
m=AllocMem_(64,$10001)
b=AllocMem_(64,$10001)
If m=0 OR b=0 Then NPrint "no memory" : End
; S404, offset 24, unpacked 1, packed 4; trailer: 4 far bits, the bit
; buffer $2080 with 9 bits: "0" + $41 (a literal "A")
Poke.l m,$53343034 : Poke.l m+4,24 : Poke.l m+8,1 : Poke.l m+12,4
Poke.w m+16,4 : Poke.w m+18,$2080 : Poke.w m+20,9
If WriteFile(0,"RAM:stc-valid") Then WriteMem 0,m,22 : CloseFile 0
Poke.l m,$53343033 : Poke.l m+8,0
If WriteFile(0,"RAM:stc-s403") Then WriteMem 0,m,22 : CloseFile 0
Poke.l m,$53343034 : Poke.l m+12,1000
If WriteFile(0,"RAM:stc-short") Then WriteMem 0,m,22 : CloseFile 0
For k=0 To 63 : Poke.b b+k,$11 : Next
r=STCLoad("RAM:stc-valid",b)
NPrint "valid: ",r=b+24," ",Peek.b(b+24)
For k=0 To 63 : Poke.b b+k,$11 : Next
r=STCLoad("RAM:stc-s403",b)
NPrint "S403: ",r<>0
For k=0 To 63 : Poke.b b+k,$11 : Next
r=STCLoad("RAM:stc-short",b)
NPrint "packed length beyond the file: ",r<>0
KillFile "RAM:stc-valid" : KillFile "RAM:stc-s403" : KillFile "RAM:stc-short"
FreeMem_ m,64 : FreeMem_ b,64
End
