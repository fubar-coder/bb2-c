; risnowlib-activate-last-not-updated (#909) - LDoSnowActivate appends
; the stopped flakes it releases to the newer drop list from its "last"
; field (offset 12) on and ends the list with $ABCD, but stores the new
; end only into the kill list's "last", not into the drop list's.  A
; second LDoSnowActivate before the next LDoSnow starts at the old end
; again and overwrites the flakes of the first.  After one LDoSnow (the
; drop list's "last" is set), two pairs of KillPoint and LDoSnowActivate:
; each KillPoint removes a blue pixel (plane 2) in row 50, and the
; stopped snow (plane 1) right above it, at x 100 and at x 140 in row
; 49, is released.  The drop list (the first long of the list table
; that InitSnowDrop returns) is read up to its end mark (at most its
; 400 entries).
; Correct: the drop list holds both released flakes.
; @rwe risnowlib
DEFTYPE .l
BitMap 0,320,256,3
t=AllocMem_(1030,$10001)
If t=0 Then NPrint "no memory" : End
lt=InitSnowDrop(0,100,0,10240)
LDoSnow 0,t
Plot 100,50,4 : Plot 100,49,2
Plot 140,50,4 : Plot 140,49,2
k=KillPoint(0,100,50)
n=LDoSnowActivate(0)
k=KillPoint(0,140,50)
n=LDoSnowActivate(0)
d=Peek.l(lt)+16
a=0 : b=0
While (Peek.w(d) AND $FFFF)<>$ABCD AND d<Peek.l(lt)+16+1600
  If Peek.w(d)=49*40+12 Then a+1
  If Peek.w(d)=49*40+17 Then b+1
  d+4
Wend
FreeMem_ t,1030
If a=1 AND b=1 Then NPrint "both flakes in the drop list"
If a=0 AND b=1 Then NPrint "first flake lost"
If b=0 Then NPrint "second flake missing"
End
