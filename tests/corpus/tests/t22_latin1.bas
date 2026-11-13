; t22_latin1 - literal characters in strings (change literal-characters):
; the Latin-1 bytes of this text other than the six umlauts are stored as
; the pair $90 cc, the umlauts as the codes 26-31; a string holds the bytes
a$="Größe à là § ° Äü"
For i=1 To Len(a$) : Print Asc(Mid$(a$,i,1))," " : Next : NPrint ""
NPrint Len(a$)
b$=String$("ß",3)+Chr$(233)
NPrint Asc(b$)," ",Len(b$)," ",b$=Chr$(223)+Chr$(223)+Chr$(223)+"é"
End
