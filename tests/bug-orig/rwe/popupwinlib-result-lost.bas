; popupwinlib-result-lost (#471) - the menu levels return the item under
; the mouse when a button is pressed, but PopUpWindow returns
; `selecteditem`, which it only clears (always 0), and it is declared a
; statement: a program cannot learn which item was chosen.
; Here: PopUpWindow used as a function on the Workbench screen with a menu
; of one item.  A vertical blank interrupt of the program drives the
; popup (the technique of popupwinlib-bchg-bfe001): once the active
; window is a borderless window without a title (the popup's) and half a
; second has passed, it sets the window's MouseX/MouseY onto the item
; (the library takes the position from there) in every frame and puts an
; IntuiMessage MOUSEMOVE (the item is highlighted) and ten frames later
; a MOUSEBUTTONS into the window's UserPort (both with a reply port of
; the program's).
; Correct: the function form compiles and returns the item's address.
; The original does not compile it (a statement).
; @fixed #471
; @rwe popupwinlib
; @pass timeout=30
DEFTYPE .l
it=AllocMem_(64,$10001)
im=AllocMem_(128,$10001)
ib=OpenLibrary_("intuition.library",0)
rp=CreateMsgPort_
If it=0 OR im=0 OR ib=0 OR rp=0 Then NPrint "no memory" : End
Poke.l it+32,$4974656D              ; "Item"
Poke.l it+4,it+32                   ; the item: its name
Poke.l im+14,rp                     ; the reply ports (one without is ignored)
Poke.l im+78,rp
Poke.l im+20,$10                    ; IDCMP_MOUSEMOVE
Poke.l im+84,8                      ; IDCMP_MOUSEBUTTONS
Poke.w im+88,$68                    ; SELECTDOWN
go=0 : n=0 : sn=0 : w=0
SetInt 5
  If go
    n=n+1
    If sn=0
      w=Peek.l(ib+52)
      If w<>0 AND n>25
        If Peek.l(w+32)<>0 OR (Peek.l(w+24) AND $800)=0 Then w=0
      Else
        w=0
      EndIf
    EndIf
    If w<>0
      Poke.w w+12,8                 ; MouseY: in the item (y 2 on)
      Poke.w w+14,10                ; MouseX
      If sn=0
        Poke.l im+44,w
        PutMsg_ Peek.l(w+86),im
        sn=1 : n=0
      Else
        If sn=1 AND n>10
          Poke.l im+108,w
          PutMsg_ Peek.l(w+86),im+64
          sn=2 : go=0
        EndIf
      EndIf
    EndIf
  EndIf
End SetInt
WbToScreen 0
go=1
r=PopUpWindow(0,it)
go=0
ClrInt 5
NPrint "messages sent ",sn
If r=it Then NPrint "result: the item" Else NPrint "result: ",r
CloseLibrary_ ib
While GetMsg_(rp) : Wend
DeleteMsgPort_ rp
FreeMem_ im,128
FreeMem_ it,64
End
