; ritooltypeslib-tooltype-array-unbounded (#855) - the tool types of the
; open icon live in the library's array of 100 longs.  NewToolType looks
; for the first free entry and clears the entry behind it without a
; count, GetIconObject copies the icon's tool types into it without a
; count: a 100th entry leaves the array without its terminating 0 and
; clears the long behind it, a 101st is written behind it.  Here
; NewToolType adds 100 entries to the cleared tool types of SYS:Prefs,
; and GetIconObject opens an icon with 101 tool types (test data
; ritooltypeslib-tooltypes-101.info, T001=1 ... T101=1).  What lies
; behind the original's array differs with the build: the program saves
; that long (the address of the array of the first icon + 400) and puts
; it back while the array at that address is still the icon's, so only
; the results and the entries are observed.
; Correct: there is no limit - all 100 NewToolType succeed and the 100th
; entry is followed by the end; the 101 tool types are all there; inject
; track finds no allocation of the library left at the end.
DEFTYPE .l

!TestCase{no_limit}
  d=GetIconObject("SYS:Prefs")
  !Require{d<>0,"icon"}
  a=Peek.l(d+54)
  o=Peek.l(a+400)
  ClearToolTypes
  n=0
  For i=1 To 100
    If NewToolType("T"+Str$(i),"1") Then n=n+1
  Next
  If Peek.l(d+54)=a Then Poke.l a+400,o : Poke.l a+396,0
  e98$=FindToolNumber(98)
  e99$=FindToolNumber(99)
  e100$=FindToolNumber(100)
  r=FreeIconObject
  d=GetIconObject("CORPUS:many")
  !Require{d<>0,"icon_many"}
  f0$=FindToolNumber(0)
  f100$=FindToolNumber(100)
  v$=FindToolValue("T101")
  If Peek.l(d+54)=a Then Poke.l a+400,o
  r=FreeIconObject
  !CheckEq{n,100,"added"}
  !CheckStrEq{e98$,"T99=1","entry_98"}
  !CheckStrEq{e99$,"T100=1","entry_99"}
  !CheckStrEq{e100$,"","entry_100"}
  !CheckStrEq{f0$,"T001=1","many_first"}
  !CheckStrEq{f100$,"T101=1","many_last"}
  !CheckStrEq{v$,"1","many_value"}
!EndTestCase
