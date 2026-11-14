; popupwinlib-bchg-bfe001 (#468) - sizing a menu level that has an item
; with a sub menu ends with BCHG #1,$bfe001 (a debug leftover): every such
; level toggles bit 1 of CIA-A port A, the power LED and the audio
; low-pass filter.
; Here: PopUpWindow on the Workbench screen with a menu of one item that
; has a sub menu.  PopUpWindow waits for a mouse button: a vertical blank
; interrupt of the program puts one IntuiMessage MOUSEBUTTONS (with a
; reply port of the program's) into the UserPort of the active window
; once that is a borderless window without a title (the popup's) and
; half a second has passed; no item is active, so the popup closes and
; returns.  The bit is read before and after PopUpWindow.
; Correct: the filter bit stays as it was.
; @fixed #468
; @rwe popupwinlib
; @pass timeout=30
DEFTYPE .l
it=AllocMem_(128,$10001)
im=AllocMem_(64,$10001)
ib=OpenLibrary_("intuition.library",0)
rp=CreateMsgPort_
If it=0 OR im=0 OR ib=0 OR rp=0 Then NPrint "no memory" : End
Poke.l it+32,$4974656D              ; "Item"
Poke.l it+4,it+32                   ; the item: its name
Poke.l it+12,it+48                  ; and its sub menu
Poke.l it+52,it+32                  ; the sub menu's item: its name
Poke.l im+14,rp                     ; the reply port (one without is ignored)
Poke.l im+20,8                      ; IDCMP_MOUSEBUTTONS
Poke.w im+24,$68                    ; SELECTDOWN
go=0 : n=0 : sent=0
SetInt 5
  If go
    n=n+1
    w=Peek.l(ib+52)
    If w<>0 AND n>25
      If Peek.l(w+32)=0 AND (Peek.l(w+24) AND $800)<>0
        Poke.l im+44,w
        PutMsg_ Peek.l(w+86),im
        go=0 : sent=1
      EndIf
    EndIf
  EndIf
End SetInt
WbToScreen 0
b0=Peek.b($BFE001) AND 2
go=1
PopUpWindow 0,it
go=0
b1=Peek.b($BFE001) AND 2
ClrInt 5
NPrint "button sent ",sent
If b0=b1 Then NPrint "filter bit kept" Else NPrint "filter bit toggled"
CloseLibrary_ ib
While GetMsg_(rp) : Wend
DeleteMsgPort_ rp
FreeMem_ im,64
FreeMem_ it,128
End
