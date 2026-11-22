; @runerrs on
; array-index-div0-runerrs (#1179) - a divisor 0 in the Word division of a
; later index, with runtime error checking (openspec/specs/compiler/
; expression-types, "The index code of a plain later index does not
; grow"): the / of two Word values is computed inline (divs.w), which
; traps on the divisor 0 as a Word division does; the debugger reports
; it as "Division by Zero", as ldivlib's Long division reported it
; before.  A valid division first.  Without the checking the trap is not
; caught (a crash, as before): no test of its own.
Dim g.b(1,100)
wv.w=5 : wy.w=2 : wz.w=0
g(1,2)=7
NPrint g(1,wv/wy)
NPrint "before"
NPrint g(1,wv/wz)
NPrint "after"
End
