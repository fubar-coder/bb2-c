; compiler-array-later-index (#1179) - a later array index, the cases the
; expression matrix does not hold (its index cells read a Dim of 300 with
; two dimensions): three dimensions, a NEWTYPE element array, a store, a
; Quick function value, a Float, a Quick calculation beyond the Quick
; range, large whole-number indexes, Word * and / beyond 32767.
; BUG(orig) (#1179): without runtime error checking a later index (the
; second and every further one) is evaluated as a Word, the first index
; and the checked build evaluate it as a Long: with y=200 g(1,y*200/2)
; computes y*200 in the Quick range (40000 overflows) and reads element
; 52768, b(1,q ASL 4) with q=1.5 shifts the Quick and reads element 24.
; Correct: every index is evaluated as a Long, with and without runtime
; error checking (spec compiler/expression-types): g(1,20000), b(1,16),
; the same element as the checked build. An index beyond 65535 still uses
; its low word without the checking.
; Each line prints the index of the element the access uses, taken from
; its address (g and h: Dim .b(1,60000) and .b(60000,1)), or the value
; stored in it (c(1,j,k) holds j*100+k, n(1,j) and v(1,j) hold j).
Function.q fq{x.q}
  Function Return x
End Function
NEWTYPE .tn
  v.l
End NEWTYPE
!TestCase{later_index}
  Dim g.b(1,60000),h.b(60000,1)
  Dim c.l(1,20,20),v.l(1,20),n.tn(1,20)
  For j.l=0 To 20
    v(1,j)=j : n(1,j)\v=j
    For k.l=0 To 20 : c(1,j,k)=j*100+k : Next
  Next
  g0.l=&g(0,0) : h0.l=&h(0,0) : o.l=0
  q.q=1.5 : y=200 : f.f=1.5 : w.w=3 : wb.w=4
  l.l=40000 : wv.w=20000 : lb.l=100000 : l4.l=40000
  jw.w=300 : kw.w=200 : wn.w=-32768 : wm.w=-1
  r.l=0
  o=&g(1,y*200/2)-g0
  r=(o-1)/2
  !CheckEq{r,20000,"i01"}   ; g(1,y*200/2)
  o=&g(1,y*300/2)-g0
  r=(o-1)/2
  !CheckEq{r,30000,"i02"}   ; g(1,y*300/2)
  o=&h(y*200/2,1)-h0
  r=o-60001
  !CheckEq{r,20000,"i03"}   ; h(y*200/2,1)
  o=&g(1,q ASL 4)-g0
  r=(o-1)/2
  !CheckEq{r,16,"i04"}   ; g(1,q ASL 4)
  o=&g(1,q*2+w+wb)-g0
  r=(o-1)/2
  !CheckEq{r,10,"i05"}   ; g(1,q*2+w+wb)
  r=c(1,q*10,5)
  !CheckEq{r,1505,"i06"}   ; c(1,q*10,5)
  r=c(1,5,q*10)
  !CheckEq{r,515,"i07"}   ; c(1,5,q*10)
  r=n(1,q*10)\v
  !CheckEq{r,15,"i08"}   ; n(1,q*10)
  v(1,q*10)=99
  r=v(1,15)
  !CheckEq{r,99,"i09_stored"}
  r=v(1,10)
  !CheckEq{r,10,"i09_untouched"}
  o=&g(1,fq{1.5}*10)-g0
  r=(o-1)/2
  !CheckEq{r,15,"i10"}   ; g(1,fq{1.5}*10)
  o=&g(1,f*10)-g0
  r=(o-1)/2
  !CheckEq{r,15,"i11"}   ; g(1,f*10)
  o=&g(1,40000)-g0
  r=(o-1)/2
  !CheckEq{r,40000,"i12"}   ; g(1,40000)
  o=&g(1,l)-g0
  r=(o-1)/2
  !CheckEq{r,40000,"i13"}   ; g(1,l)
  o=&g(1,wv*2)-g0
  r=(o-1)/2
  !CheckEq{r,40000,"i14"}   ; g(1,wv*2)
  o=&g(1,wv*3)-g0
  r=(o-1)/2
  !CheckEq{r,60000,"i15"}   ; g(1,wv*3)
  o=&g(1,lb/2)-g0
  r=(o-1)/2
  !CheckEq{r,50000,"i16"}   ; g(1,lb/2)
  o=&g(1,jw*kw)-g0
  r=(o-1)/2
  !CheckEq{r,60000,"i17"}   ; g(1,jw*kw)
  o=&g(1,wn/wm)-g0
  r=(o-1)/2
  !CheckEq{r,32768,"i18"}   ; g(1,wn/wm)
  o=&g(1,jw*kw/2)-g0
  r=(o-1)/2
  !CheckEq{r,30000,"i19"}   ; g(1,jw*kw/2)
  o=&g(1,wn/wm/2)-g0
  r=(o-1)/2
  !CheckEq{r,16384,"i20"}   ; g(1,wn/wm/2)
  o=&g(1,l4*2)-g0
  r=(o-1)/2
  !CheckEq{r,14464,"i21"}   ; g(1,l4*2)
!EndTestCase
