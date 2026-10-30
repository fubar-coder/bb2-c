; rimapshapeslib-group-no-limit (#904) - AddGroupMember appends an 8-byte
; member {map shape address, dx, dy} and the end mark to the 512-byte
; group block without counting: the 64th member's end mark lies behind
; the block, the 65th and 66th member are written behind it.  The program
; runs through inject pad (64 zeroed bytes behind every allocation of the
; process, so the writes stay in memory of the program); members 0-65 use
; map shape 7, the 66th member's address is looked for 520 bytes from the
; group's start.
; Correct: the group holds at most what fits into its 512 bytes, nothing
; is written behind the block.
; @rwe rimapshapeslib
; @pass runwrap=@INJECT@|pad
DEFTYPE .l
SetNumMapShapes 10
g=ResetGroup(16,16)
For i=0 To 65
  AddGroupMember 7,1,1
Next
If Peek.l(g+520)=MapShapeAddr(7) Then NPrint "written behind the block" Else NPrint "nothing behind the block"
End
