; ritooltypeslib-seticontype-noicon-returns-arg (#852) - SetIconType
; returns at once when no icon is open, with the argument still in d0:
; SetIconType 3 without an icon returns 3, which a program takes for
; success.  No icon is opened here.
; Correct: without an icon SetIconType returns 0.
; @fixed #852
; @rwe ritooltypeslib
NPrint SetIconType(3)
End
