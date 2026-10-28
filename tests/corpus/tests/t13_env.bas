; t13_env - environlib: values read at program start (environment
; dependent, but the same in every run of the same emulator setup)
NPrint "wb ",WBWidth,"x",WBHeight,"x",WBDepth
NPrint "viewmode ",Hex$(WBViewMode)," ext ",Hex$(WBExtViewMode)
NPrint "cpu ",Processor," vbr ",Hex$(cr_vbr)," exec ",ExecVersion
w.l=WBWidth*2+WBHeight : d.w=WBDepth+Processor
NPrint "expr ",w," ",d," ",ExecVersion>=33
q.q=WBViewMode : f.f=WBWidth/3
NPrint "conv ",q," ",f
End
