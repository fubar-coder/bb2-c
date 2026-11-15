; arrayslib-dim-65535 (#1037) - an array or list has at most 65535
; elements: arrmult counts the elements of a dimension as a word (ADDQ
; #1 on the highest index), so the highest index 65535 gives 0 elements,
; and every access lies outside the empty block.  The error check of a
; dimension (aarraychk, "Array Dimension out of Range") refuses only
; 65536 and more.
; Here runtime error checking is on: Dim with the highest index 65534
; (65535 elements, the largest array), then 65535.
; Correct: the error check refuses the dimension 65535.
; @fixed #1037
; @configs clib
; @pass runtime_errors=on
; @pass timeout=30
Dim a.b(65534)
a(65534)=7
NPrint "65534 ",a(65534)
Dim b.b(65535)
NPrint "65535 dimmed"
End
