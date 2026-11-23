; a child of editlib-edit-eof: Edit$ and Edit at the end of its input
; (the parent starts it with the input NIL:)
NPrint "edit$ ",Len(Edit$(10))
NPrint "edit$dflt ",Len(Edit$("ab",5))
g.f=Abs(g) : f.f=Edit(5) : NPrint "edit ",f
End
