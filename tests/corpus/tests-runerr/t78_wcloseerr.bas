; t78_wcloseerr - windowslib error check wchk: drawing after CloseWindow
WbToScreen 0
Window 0,20,30,200,80,$000E,"Closed",1,2
DefaultOutput
CloseWindow 0
NPrint "before"
WPlot 10,10,1
NPrint "after"
End
