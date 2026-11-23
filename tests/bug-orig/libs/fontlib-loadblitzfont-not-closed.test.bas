; fontlib-loadblitzfont-not-closed (#150) - LoadBlitzFont opens the font
; with OpenDiskFont (TextAttr size 8) and refuses it when it is not 8x8
; or proportional ("Can't Open Font", a runtime error also without the
; debugger) without closing it: the font stays open (tf_Accessors not
; counted down) after the program.
; Here: diamond.font of the Workbench (12 and 20 points only): diskfont
; scales the 12 point font to the 8 points asked for (5 pixels wide,
; proportional), which LoadBlitzFont refuses.  SetErr catches the
; runtime error, then the tf_Accessors (+30) of the 8 point diamond
; fonts in the graphics font list (GfxBase +140, tf_YSize +20) are added
; up.
; Correct: the refused font is closed again: accessors 0.  The original
; leaves it open: 1.
DEFTYPE .l

!TestCase{refused_font}
  SetErr
    g.l=OpenLibrary_("graphics.library",0)
    n.l=0
    f.l=FindName_(g+140,"diamond.font")
    While f
      If Peek.w(f+20)=8 Then n=n+Peek.w(f+30)
      f=FindName_(f,"diamond.font")
    Wend
    !CheckEq{n,0,"diamond_8_accessors"}
    CloseLibrary_ g
    End
  End SetErr
  LoadBlitzFont 1,"diamond.font"
  !Check{0,"not_refused"}
!EndTestCase
