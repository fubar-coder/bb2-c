; compiler-userlibs-doubleslash (#2) - loadblitzlibs builds the path of a
; library directory from libsdir "blitz2:userlibs/" and appends another
; '/': "blitz2:userlibs//bugorig" is blitz2:bugorig for AmigaDOS, the
; library in blitz2:userlibs/bugorig is never loaded.
; Correct: the program uses the function of that library (42).
; @pass file=blitz2/userlibs/bugorig/bugorig.obj|@USERLIB@
NPrint BugOrigUserLib
End
