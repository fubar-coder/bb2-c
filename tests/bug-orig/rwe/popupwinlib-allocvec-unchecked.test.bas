; popupwinlib-allocvec-unchecked (#467) - each menu level allocates its
; 64-byte structure with AllocVec and does not check the result: with a
; failed allocation the structure is at address 0 - the item list is
; read from address 10, the sizes and the window data are written to
; addresses 14-39 (exception vectors), a window is opened for the
; garbage list and waits for a mouse button (the session ends in a
; timeout).  The program runs through inject with the third allocation
; of 68 bytes with MEMF_PUBLIC|MEMF_CLEAR failing (AllocVec(64): 64 + its
; size field; the flags keep memlib's buckets of list headers, also 68
; bytes, out): WbToScreen makes two such allocations of the system's
; before (with the original libraries and the C ones).
; A vertical blank interrupt of the program ends a popup window that
; opens all the same (the fault missed the AllocVec) with a MOUSEBUTTONS
; message, as popupwinlib-bchg-bfe001 does: "button sent" 1 then says
; the test did not hit the allocation.
; Correct: PopUpWindow fails without a window and returns, the program
; goes on.
DEFTYPE .l

!TestCase{allocvec_fails}
  it=AllocMem_(64,$10001)
  im=AllocMem_(64,$10001)
  ib=OpenLibrary_("intuition.library",0)
  rp=CreateMsgPort_
  !Require{it<>0 AND im<>0 AND ib<>0 AND rp<>0,"memory"}
  Poke.l it+32,$4974656D              ; "Item"
  Poke.l it+4,it+32
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
  go=1
  PopUpWindow 0,it
  go=0
  ClrInt 5
  !CheckEq{sent,0,"button_sent"}
  CloseLibrary_ ib
  While GetMsg_(rp) : Wend
  DeleteMsgPort_ rp
  FreeMem_ im,64
  FreeMem_ it,64
!EndTestCase
