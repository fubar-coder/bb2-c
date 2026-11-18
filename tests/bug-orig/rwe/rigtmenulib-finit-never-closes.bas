; rigtmenulib-finit-never-closes (#690) - the library's init opens
; gadtools.library; finit tests the base the wrong way round (TST.l:BNE
; past the CloseLibrary) and never closes it.  The program builds a
; menu strip, so that the object's free routine (FreeMenus) runs at the
; end too; it runs through inject, which counts the opens minus the
; closes of gadtools.library by the process (opencount=).
; Correct: gadtools.library is closed again (+0); the original leaves
; it +1.
; @fixed #690
; @rwe rigtmenulib
; @pass runwrap=@INJECT@|opencount=gadtools.library
WbToScreen 0
GTMenuTitle 0,0,"Project"
GTMenuItem 0,0,0,0,"Open","O"
NPrint CreateMenuStrip(0)
End
