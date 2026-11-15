; utillib-deletemsgport-foreign (#1041) - DeleteMsgPort takes any address
; as a port: a port that CreateMsgPort did not make (not in utillib's
; port list) is unlinked through its (foreign) list pointers, its signal
; freed and its memory freed as a utillib block, which corrupts memory.
; It is a programming error and the original has no error check for it
; (with runtime error checking only memlib reports "Unable to Free
; Memory", after the damage).
; Here runtime error checking is on: a port of CreateMsgPort is deleted,
; then a port built by hand in a string (an empty message list, signal
; 0, no name).
; Correct: the own port passes; the error check refuses the other one
; with "Port Not Created By CreateMsgPort".
; @fixed #1041
; @configs clib
; @pass runtime_errors=on
; @pass timeout=30
DEFTYPE .l
p=CreateMsgPort("")
DeleteMsgPort p
NPrint "own port deleted"
m$=String$(Chr$(0),48) : q=&m$+8
Poke.b q+8,4
Poke.l q+20,q+24 : Poke.l q+28,q+20
DeleteMsgPort q
NPrint "foreign port deleted"
End
