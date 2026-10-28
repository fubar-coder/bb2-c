; t07_pre - macros, constants, conditional compiling
#size=10
#name=1
Macro sq
  (`1*`1)
End Macro
Macro sumthree
  `1+`2+`3
End Macro
NPrint "const ",#size," ",#size*2
NPrint "macro ",!sq{7}," ",!sumthree{1,2,3}
CNIF #size>5
NPrint "cnif big"
CELSE
NPrint "cnif small"
CEND
CNIF #size=3
NPrint "never"
CEND
CSIF "abc"="abc"
NPrint "csif same"
CEND
End
