; compiler-constdiv-nonzero (#1044) - constant divisions and Mods that the
; fix of #1044 lets through: a divisor that is not 0, of each type (also
; a Long divisor whose low word is 0), nested folds, a dividend 0, a
; #constant, a Long Mod (not folded), and a division by a variable that
; holds 0 in a branch that is not run (only a constant divisor is checked
; at compile time).
; Correct: the program compiles and prints the quotients and remainders.
; A regression test of the fix of #1044: the original already gives this
; (correct) result.
a.w=7/2
NPrint a
a.w=-7/2
NPrint a
a.w=0/5
NPrint a
a.w=(8/2)/(4/2)
NPrint a
b.b=100/7
NPrint b
d.l=7/65536
NPrint d
d.l=100000/-3
NPrint d
d.l=70001 Mod 7
NPrint d
e.q=7/2
NPrint e
f.f=1/4
NPrint f
f.f=7/0.5
NPrint f
#c=10/3
NPrint #c
z.w=0
If z<>0 Then a=5/z
NPrint a
End
