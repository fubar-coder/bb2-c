; compiler-pushput-leak (#14) - pushput (assignments with an address
; computed before the expression, Select) puts the code that pushdo
; saved back and frees the save buffer; when the object buffer is full
; already (nomemleft) or overflows while copying back, it returns
; without FreeMem: the buffer leaks.  The leak needs an overflow between
; pushdo and pushput, i.e. while the expression is compiled.
; Here the object buffer of the default options is 1024 bytes (tedstub
; objbuf=): the assignments to an array element with long expressions
; overflow it in the first pass, in the middle of an expression; with
; "Make Smallest Code" of the default options blitz2 then sizes the
; buffer to the code and compiles again (no "Not Enough Object Memory").
; inject records blitz2's allocations (track).
; Correct: blitz2 leaves no allocation behind.
; @fixed #14
; @pass jobline=objbuf=1024
; @pass wrap=@INJECT@|track
DEFTYPE .l
Dim a(10)
i=1 : b=2
a(i)=b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b
a(i)=b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b
a(i)=b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b
a(i)=b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b
a(i)=b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b
a(i)=b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b+b
NPrint a(1)
End
