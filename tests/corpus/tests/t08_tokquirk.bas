; t08_tokquirk - TED 1.1 tokeniser quirks (see tools/bb2tok.py)
; a keyword followed by a digit is still a keyword: "add3" = ADD + 3,
; so it cannot be a name.  After '.', '_' or '\' no keyword is searched.
my_print=5 : NPrint "underscore ",my_print
; "sub1" would be SUB + 1 (asm mnemonic), "add3" ADD + 3
x1=7 : NPrint "digit name ",x1
v.w=$10 : NPrint "hex ",v
End
