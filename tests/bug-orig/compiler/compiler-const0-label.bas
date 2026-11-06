; compiler-const0-label (#977) - makealab2 decides whether a label exists
; only by its value (pc): a constant of value 0 with the name of a label
; counts as "label not defined yet", and a reference is chained in front
; of the constant's own value (refs == 1 marks the constant). The
; definition of the label (handlelab2) then walks that chain from
; address 1; here the compile ends with "Label reference out of context"
; at a line number that is garbage (3585), without the label ever having
; been defined by the program.
; Correct: the name clash is refused at the Goto (line 12) with the error
; for a name used as a constant.
#foo=0
Goto foo
foo:
NPrint "label"
End
