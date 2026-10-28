; t136_cookieerr - blitlib error check maxchk: Blit of a shape without a
; cookie (AutoCookie Off)
BitMap 0,64,32,2
Use BitMap 0
AutoCookie Off
GetaShape 0,0,0,32,16
NPrint "cookie ",Peek.l(Addr Shape(0)+18)
NPrint "before"
Blit 0,0,0
NPrint "after"
End
