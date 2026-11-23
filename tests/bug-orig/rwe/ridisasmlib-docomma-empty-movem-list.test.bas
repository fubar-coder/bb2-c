; ridisasmlib-docomma-empty-movem-list (#615) - the comma between the
; operands is set when the operand pointer (a5) differs from the line
; position (a2) after the source operand; the register list code moves
; a5 through its table, so MOVEM with an empty register list (mask 0)
; still gets a comma: "movem.l  ,-(a7)".
; Correct: no comma when the source wrote nothing: "movem.l  -(a7)"; a
; list that is not empty keeps its comma.
DEFTYPE .l

!TestCase{empty_list}
  Dim c.w(3)
  c(0)=$48E7 : c(1)=0
  a$=DisAssemble$(&c(0))
  !CheckStrEq{a$,"movem.l  -(a7)                    ","empty"}
  c(1)=$C000
  b$=DisAssemble$(&c(0))
  !CheckStrEq{b$,"movem.l  d0-d1,-(a7)              ","not_empty"}
!EndTestCase
