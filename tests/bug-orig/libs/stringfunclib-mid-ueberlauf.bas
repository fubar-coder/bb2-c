; stringfunclib-mid-ueberlauf (#286) - Mid$(s$,start,len) compares
; start-1+len with the length of the string signed after the addition:
; with len = $7FFFFFFF the sum overflows, counts as "fits", and the result
; gets len bytes - far beyond the string and the string buffer (printed,
; the program writes 2 GB of memory; here only its length is shown).
; Correct: the length is cut to the rest of the string ("ello", 4).
; @configs clib
; @pass timeout=45
a$="Hello"
NPrint Len(Mid$(a$,2,$7FFFFFFF))
End
