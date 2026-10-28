; t51_cliargs - cliargslib: FromCLI, NumPars, Par$; ParPath$ (Workbench
; only, from the CLI only without error checks).  The corpus runner starts
; programs without arguments.
NPrint "fromcli ",FromCLI
n=NumPars
NPrint "numpars ",n
For i=1 To n
  p$=Par$(i)
  NPrint i," [",p$,"] ",Len(p$)
Next
If n>0
  NPrint "concat [",Par$(1)+"|"+Par$(n),"]"
  ; error check: only from Workbench; without: NameFromLock(0)
  NPrint "parpath [",ParPath$(1,0),"]"
  NPrint "parpath [",ParPath$(n,1),"]"
EndIf
NPrint "end"
End
