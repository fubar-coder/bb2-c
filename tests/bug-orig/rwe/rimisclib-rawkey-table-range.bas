; rimisclib-rawkey-table-range (#744) - ASCRaw and RawASC use a table of
; 114 raw key codes without checking its range: ASCRaw of a key up code
; (128-255) or of 114 and more returns a byte of the code behind the
; table, RawASC of a character not in the table (an upper case letter)
; searches on behind it until it finds the byte somewhere, and RawASC("")
; finds the 0 of a key without a character (14).
; Correct: ASCRaw outside 0..113 gives the character 0, RawASC of a
; character not in the table and of "" gives -1.
; @fixed #744
; @rwe rimisclib
; @pass timeout=30
NPrint Asc(ASCRaw(32))," ",Asc(ASCRaw(160))," ",Asc(ASCRaw(200))
NPrint RawASC("a")," ",RawASC("A")," ",RawASC("")
End
