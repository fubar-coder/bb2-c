; plain - the run-output cases of AB3 mode without a compiler state of
; their own (the default OPTIMIZE and SYNTAX, no header), in one file so
; that AmiBlitz3 compiles them in one session. Each case was a case of
; tests/ab3/CMakeLists.txt and has its name. A value a procedure printed
; is passed in zo$ (a string) or zn (a Long).
!TestBatchInit
  zo$="" : zn.l=0
!EndTestBatchInit
; funcstmt_num: a Function called as a statement: compiled as a
; Statement's call, the result discarded (AmiBlitz3.ab3:4416-4426, ;BPL.w
; illprocerr)
Function.l myadd{a.l,b.l}
  SHARED zn
  zn=a + b
  Function Return a + b
End Function
; funcstmt_str: a string Function called as a statement: its result is
; left in the string buffer, the strings of the program stay intact
Function.s mygreet{n$}
  SHARED zo$
  zo$="hello "+n$
  Function Return "<" + n$ + ">"
End Function
; funcstmt_nopar: a Function without parameters called as a statement,
; also one that ends without Function Return
Function.w mycount{}
  SHARED n.w
  n = n + 1
  Function Return n
End Function
Function.l mynothing{}
End Function
!TestCase{plus}
  ; the + prefix of a statement (no type check): compiled as without it
  b.w = 3
+a.l = b.w
  !CheckEq{a,3,"l1"}
!EndTestCase
!TestCase{funcstmt_num}
  myadd{2,3}
  !CheckEq{zn,5,"l1"}
  x.l = myadd{4,5}
  !CheckEq{zn,9,"l2"}
  zs$="sum "+Str$(x)
  !CheckStrEq{zs$,"sum 9","l3"}
!EndTestCase
!TestCase{funcstmt_str}
  a$ = "abc"
  mygreet{"world"}
  !CheckStrEq{zo$,"hello world","l1"}
  !CheckStrEq{"next","next","l2"}
  b$ = mygreet{a$}
  !CheckStrEq{zo$,"hello abc","l3"}
  zs$=a$+"x"+b$
  !CheckStrEq{zs$,"abcx<abc>","l4"}
  mygreet{a$ + "def"}
  !CheckStrEq{zo$,"hello abcdef","l5"}
  !CheckStrEq{a$,"abc","l6"}
!EndTestCase
!TestCase{funcstmt_nopar}
  mycount{} : mycount{} : mynothing{} : zs$="count "+Str$(mycount{})
  !CheckStrEq{zs$,"count 3","l1"}
!EndTestCase
