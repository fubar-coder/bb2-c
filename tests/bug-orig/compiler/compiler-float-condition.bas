; compiler-float-condition (#1187) - If, While and Until on a Float value
; test the value with mathffp SPTst (jsr -48(a6), blitz2.s:12966
; thetst5), which takes its argument in d1; the compiler has the value in
; d0 and does not copy it (doif2, blitz2.s:13299), so the branch follows
; what the code before left in d1.  The program sets d1 itself before
; every condition (x=a+b leaves b=0 in d1, x=a+c leaves c=7), each time
; to the value that makes the original take the wrong branch, so the
; result does not depend on the library set.  Correct: a Float condition
; is true when the value is not 0, whatever is in d1, like the other
; types.  Comparisons and Quick conditions are tested with tst and stay
; right (q-0.5 is a Float expression in the port since #1177: the
; constant with a fraction is a Float; in the original a Quick).
; @fixed #1187
a.l=5 : b.l=0 : c.l=7 : e.l=0 : x.l=0 : n.l=0
f.f=1.5 : z.f=0 : m.f=-2.5 : q.q=0.5 : qz.q=0
x=a+b : If f Then NPrint "01 f T" Else NPrint "01 f F"
x=a+c : If z Then NPrint "02 z T" Else NPrint "02 z F"
x=a+b : If m Then NPrint "03 m T" Else NPrint "03 m F"
x=a+b : If 0.5 Then NPrint "04 0.5 T" Else NPrint "04 0.5 F"
x=a+c : If 0.0 Then NPrint "05 0.0 T" Else NPrint "05 0.0 F"
x=a+c : If 1.5-1.5 Then NPrint "06 1.5-1.5 T" Else NPrint "06 1.5-1.5 F"
x=a+b : If 1.5+1.5 Then NPrint "07 1.5+1.5 T" Else NPrint "07 1.5+1.5 F"
x=a+b : If f+z Then NPrint "08 f+z T" Else NPrint "08 f+z F"
x=a+c : If f-f Then NPrint "09 f-f T" Else NPrint "09 f-f F"
x=a+c : If z*f Then NPrint "10 z*f T" Else NPrint "10 z*f F"
x=a+c : If Abs(z) Then NPrint "11 Abs(z) T" Else NPrint "11 Abs(z) F"
x=a+b : If Abs(m) Then NPrint "12 Abs(m) T" Else NPrint "12 Abs(m) F"
x=a+b : If -f Then NPrint "13 -f T" Else NPrint "13 -f F"
x=a+b
If f
  NPrint "14 block f T"
Else
  NPrint "14 block f F"
EndIf
x=a+c
If z
  NPrint "15 block z T"
Else
  NPrint "15 block z F"
EndIf
x=a+c : If f<>0 Then NPrint "16 f<>0 T" Else NPrint "16 f<>0 F"
x=a+c : If z<>0 Then NPrint "17 z<>0 T" Else NPrint "17 z<>0 F"
x=a+b : If z=0 Then NPrint "18 z=0 T" Else NPrint "18 z=0 F"
x=a+b : If f=z Then NPrint "19 f=z T" Else NPrint "19 f=z F"
x=a+b : If q Then NPrint "20 q T" Else NPrint "20 q F"
x=a+c : If qz Then NPrint "21 qz T" Else NPrint "21 qz F"
x=a+c : If q-0.5 Then NPrint "22 q-0.5 T" Else NPrint "22 q-0.5 F"
; While f: 3 rounds (the original: 0, d1=0)
n=0 : f=1.5
x=a+b : While f : n+1 : If n=3 Then f=0
x=a+b : Wend
NPrint "23 While f rounds ",n
; While z: 0 rounds (the original: until c is 0 at round 3)
n=0 : z=0 : c=7
x=a+c : While z : n+1 : If n=3 Then c=0
x=a+c : Wend
NPrint "24 While z rounds ",n
c=7
; Until f with f 0, 0, 1.5: 3 rounds (the original: until e is 7 at round 5)
n=0 : f=0 : e=0
Repeat : n+1 : If n=3 Then f=1.5
If n=5 Then e=7
x=a+e : Until f
NPrint "25 Until f rounds ",n
; Until z with z 0, 0, 0, 1: 4 rounds (the original: 1, d1=7)
n=0 : z=0
Repeat : n+1 : If n=4 Then z=1
x=a+c : Until z
NPrint "26 Until z rounds ",n
End
