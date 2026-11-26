; select-fraction-runerr - a Select whose value rule P+ computes in 48.16 (#1216;
; decisions "the Select value with a fraction", "only a 48.16 Select is
; wide"): the value is kept as 48.16 and each Case is compared with it
; exactly, a Float Case in Float; a Quick, a whole and a Float Select
; compare as before.  With runtime error checking on (select-fraction.test.bas
; without it).
; The original (blitz2 1.95) gives the same values (checked once, so the
; test has no key and runs on the port alone).
; Correct: each Case below matches as its check says.
!TestCase{cases}
  q.q=1.5 : l.l=2 : p.q=0.5 : r.l=0 : f.f=1.5 : w.w=3 : one.l=1
  Select q*l
    Case 3 : r=1
    Case 2 : r=2
    Default : r=9
  End Select
  !CheckEq{r,1,"product_three"}
  Select q*one
    Case 1 : r=1
    Case 1.5 : r=2
    Default : r=9
  End Select
  !CheckEq{r,2,"case_fraction"}
  l=100000
  Select q*l+p
    Case 150000 : r=1
    Case 150000.5 : r=2
    Default : r=9
  End Select
  !CheckEq{r,2,"wide_fraction"}
  Select q*one
    Case f : r=1
    Default : r=9
  End Select
  !CheckEq{r,1,"float_case"}
  Select q*one
    Case 2 : r=1
    Default : r=9
  End Select
  !CheckEq{r,9,"default"}
  Select q*one+p
    Case 2
      Select q*one*2
        Case 3 : r=3
        Default : r=8
      End Select
    Default : r=9
  End Select
  !CheckEq{r,3,"nested"}
  ; the forms that stay as before: a Quick, a 16.16 and a whole Select
  Select q
    Case 1.5 : r=1
    Default : r=9
  End Select
  !CheckEq{r,1,"quick"}
  Select q+p
    Case 2 : r=1
    Default : r=9
  End Select
  !CheckEq{r,1,"quick_sum"}
  Select w
    Case 3 : r=1
    Default : r=9
  End Select
  !CheckEq{r,1,"whole"}
!EndTestCase
