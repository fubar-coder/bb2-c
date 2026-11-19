; rimapshapeslib-group-full-check (#904) - AddGroupMember appends an 8-byte
; member and the end mark to the 512-byte group block without counting:
; the 64th member's end mark lies behind the block.  Here with runtime
; error checking: a program that adds more members than the group holds
; (63 and the end mark) is told so.  The program runs through inject pad
; (64 zeroed bytes behind every allocation of the process, so the end
; mark stays in memory of the program).
; Correct: the error check refuses the 64th member: "Group holds at most
; 63 members".
; @rwe rimapshapeslib
; @pass runwrap=@INJECT@|pad
; @pass runtime_errors=on
DEFTYPE .l
SetNumMapShapes 10
g=ResetGroup(16,16)
NPrint "before"
For i=1 To 64
  AddGroupMember 7,1,1
Next
NPrint "64 members added"
End
