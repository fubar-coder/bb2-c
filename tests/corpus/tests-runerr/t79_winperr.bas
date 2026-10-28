; t79_winperr - windowslib: MButtons with a closed input window (runtime
; error from the normal code; the original jumps into its error check area)
WbToScreen 0
Window 0,20,30,200,80,$000E,"Input",1,2
DefaultOutput
CloseWindow 0
NPrint "before"
a=MButtons
NPrint "after ",a
End
