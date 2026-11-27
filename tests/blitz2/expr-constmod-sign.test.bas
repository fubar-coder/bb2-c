; expr-constmod-sign (#1228) - a whole Mod of constants with a negative
; operand types the expression by the value the code computes.
; The code computes a Mod by today's sign rule, the divisor added to a
; negative remainder: -1 Mod 40000 is 39999, a Long beyond a Word and a
; Quick.  The port's typing pass (rule P+, #1216) took the value with C's
; sign rule (-1, a Byte), so w+(-1 Mod 40000) added in a Word (4463 under
; Print), (-1 Mod 40000)+b printed -25437, q+(-1 Mod 40000) wrapped in
; 16.16 (-25536.5) and If v+(-1 Mod 40000) with v=25537 was false.
; Correct, as blitz2 1.95 gives it: the constant typed by the code's value,
; the operation in a Long (48.16 with the Quick): 69999, 40099, 39999
; (Print takes the whole part beyond the Quick range), true.  The other
; lines (among them -3 Mod 7<q, 4<1.5: false) are the same in both and
; stay as checks.
!TestCase{printed}
  b.b=100 : c.b=2 : w.w=30000 : q.q=0.5
  NPrint b+(-1 Mod 200)
  NPrint (-1 Mod 200)+b
  NPrint c*(-1 Mod 200)
  NPrint w+(-1 Mod 40000)
  NPrint (-1 Mod 40000)+b
  NPrint q+(-1 Mod 40000)
  NPrint -6 Mod 4
!EndTestCase
!TestCase{assigned}
  b.b=57 : w.w=30000 : n.w=-30000 : v.w=25537 : q.q=1.5
  rw.w=0 : rq.q=0 : rl.l=0
  If b+(-1 Mod 200) Then NPrint "if true" Else NPrint "if false"
  If v+(-1 Mod 40000) Then NPrint "if true" Else NPrint "if false"
  If -3 Mod 7<q Then NPrint "lt true" Else NPrint "lt false"
  If -3 Mod 7<1.5 Then NPrint "lt true" Else NPrint "lt false"
  rw=w+(-1 Mod 40000) : NPrint rw
  rq=n+(-1 Mod 40000) : NPrint rq
  rl=w+(-1 Mod 40000) : NPrint rl
!EndTestCase
