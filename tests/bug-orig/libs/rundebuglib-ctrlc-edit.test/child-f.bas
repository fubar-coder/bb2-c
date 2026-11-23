; a child of rundebuglib-ctrlc-edit: it sends itself Ctrl-C (exec SetSignal
; of SIGBREAKF_CTRL_C) and reads a number with Edit from its input (the
; parent's input.txt)
SetSignal_ $1000,$1000
f.f=Edit(5)
SetSignal_ 0,$1000
NPrint "edit not stopped"
End
