; rishapeslib-cludged-shape-pop.test/victim.bas: the program run from the editor
; by the job line of the test; see the test for what it shows
DEFTYPE .l
n$="CORPUS:s.le"
f=Open_(&n$,1005)
If f=0 Then NPrint "no file" : End
m=AllocMem_(1024,$10002)
r=Read_(f,m,1024)
Close_ f
LECludgeShapes 0,0,m,0
NPrint "linked ",ShapeWidth(0)
LELoadShapes 0,0,"CORPUS:s.le"
NPrint "loaded"
End
