; popupwinlib-v36-menu (#1140) - every program with the library crashes
; at start-up on Kickstart 1.3 (init's CreateMsgPort, exec V36); a menu
; level allocates its structure with AllocVec and opens its window with
; OpenWindowTagList (intuition V36), all without a version check.
; Here: the menu of popupwinlib-result-lost (one item on the Workbench
; screen, driven by a vertical blank interrupt of the program: once the
; active window is the popup's, MouseX/MouseY onto the item in every
; frame, an IntuiMessage MOUSEMOVE into the window's UserPort and ten
; frames later a MOUSEBUTTONS), with a reply port built by hand
; (CreateMsgPort is V36 as well).  PopUpWindow as a statement: the
; original does not compile the function form (#471), whose result
; popupwinlib-result-lost checks.
; Correct: on every machine the menu window opens (below intuition V36
; with OpenWindow), both messages reach it and the mouse button closes
; the menu: PopUpWindow returns.
; @fixed #1140
; @rwe popupwinlib
; @machine a500 a500-204 a1200
; @pass timeout=30
DEFTYPE .l
NPrint "before"
it=AllocMem_(64,$10001)
im=AllocMem_(128,$10001)
ib=OpenLibrary_("intuition.library",0)
rp=AllocMem_(34,$10001)
If it=0 OR im=0 OR ib=0 OR rp=0 Then NPrint "no memory" : End
sg=AllocSignal_(-1)
If sg<0 Then NPrint "no signal" : End
Poke.b rp+8,4                       ; NT_MSGPORT, PA_SIGNAL
Poke.b rp+15,sg
Poke.l rp+16,FindTask_(0)
Poke.l rp+20,rp+24                  ; an empty message list
Poke.l rp+28,rp+20
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
PopUpWindow 0,it
go=0
ClrInt 5
NPrint "messages sent ",sn
CloseLibrary_ ib
While GetMsg_(rp) : Wend
FreeSignal_ sg
FreeMem_ rp,34
FreeMem_ im,128
FreeMem_ it,64
NPrint "after"
End
