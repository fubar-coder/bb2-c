; freqlib-64-lines (#160) - FileRequest$ shows (window height - 85) / 8
; list lines with no limit, but it has only 64 line gadgets at the end of
; its data block.  The first window is screen height - 40 high: on a
; screen of 645 lines or more the layout writes the surplus line gadgets
; (144 bytes each) behind the data block, into the program.
; Here: the RTG variant of the emulator (rtg=on: the uaegfx board; the
; native modes have no screen that tall).  The program opens a uaegfx
; screen of 1024 x 2048 itself (OpenScreenTagList with the display ID that
; BestModeIDA gives for 1024 x 768), makes it the used screen (FindScreen)
; and calls FileRequest$ on it: window 2008 lines, 240 list lines, 176 too
; many, about 25 KB written behind the data block, which lies near the end
; of the program's hunk.  inject runs the program with guardseg (32 KB of a
; pattern behind the program's hunk, 64 bytes behind every other
; allocation, checked at FreeMem and at the end) and answers the requester
; with its CANCEL gadget (click=).
; Correct: the requester opens and is cancelled, nothing is written behind
; an allocation.
; Judged in port (blitz2-c with the CD's libraries) while the bug is open:
; what the overrun hits depends on the layout of the library.  The C
; library keeps its parameter block (fq_args, the screen among it) right
; behind the data block; the overrun destroys the screen pointer before
; OpenWindow, which then fails or crashes the machine (both seen).
; @configs port
; @pass rtg=on
; @pass runwrap=@INJECT@|guardseg click=FQ/CANCEL
; @pass timeout=120
DEFTYPE .l
MaxLen p$=160
MaxLen f$=64
p$="RAM:"
f$="x"
s$="bo160"
t=AllocMem_(128,$10001)
Poke.l t,$80000004 : Poke.l t+4,1024        ; BIDTAG_NominalWidth
Poke.l t+8,$80000005 : Poke.l t+12,768      ; BIDTAG_NominalHeight
Poke.l t+16,$80000008 : Poke.l t+20,8       ; BIDTAG_Depth
Poke.l t+24,0
id=BestModeIDA_(t)
Poke.l t,$80000032 : Poke.l t+4,id          ; SA_DisplayID
Poke.l t+8,$80000023 : Poke.l t+12,1024     ; SA_Width
Poke.l t+16,$80000024 : Poke.l t+20,2048    ; SA_Height
Poke.l t+24,$80000025 : Poke.l t+28,8       ; SA_Depth
Poke.l t+32,$80000028 : Poke.l t+36,&s$     ; SA_Title
Poke.l t+40,$8000003A : Poke.l t+44,t+120   ; SA_Pens: the new look
Poke.w t+120,-1
Poke.l t+48,0
scr=OpenScreenTagList_(0,t)
If scr=0 Then NPrint "no screen" : FreeMem_ t,128 : End
h=Peek.w(scr+14)
If h>=645 Then NPrint "tall screen" Else NPrint "screen of ",h," lines"
FindScreen 0,"bo160"
r$=FileRequest$("FQ",p$,f$)
NPrint "cancel ",Len(r$)
CloseScreen_ scr
FreeMem_ t,128
End
