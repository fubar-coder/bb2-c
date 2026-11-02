; #__debug without runtime error checking: 0; Stop inside CNIF #__debug is
; skipped; a source's own #__debug replaces the predefined one (as in
; AmiBlitz3, where it is an ordinary constant defined before the source)
NPrint #__debug
CNIF #__debug : Stop : CEND
CNIF #__debug=0
NPrint "release build"
CEND
#__debug=5
NPrint #__debug
NPrint "end"
End
