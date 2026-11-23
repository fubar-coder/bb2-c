; bb-blitlib-btblockimage-word-offset (#427) - BTBlockImage adds image x
; 224 to image_addr as a word (ADD.w): from image 147 on the offset is
; negative, image 150 is read 65536 - 33600 bytes before image_addr.
; Here: the image table lies in the middle of a cleared chip block: image
; 150 (mask and plane 0 all set, planes 1-5 clear) is inside it, and so are
; the zeros 31936 bytes before image_addr.
; Correct: image 150 is drawn, pixel 40,40 has colour 1 (the bug draws
; the zeros: 0).
DEFTYPE .l

!TestCase{image_150}
  m=AllocMem_(80000,$10002)
  !Require{m<>0,"memory"}
  img=m+40000
  p=img+150*224
  For i=0 To 63 Step 4 : Poke.l p+i,-1 : Next
  BitMap 0,320,64,6
  Buffer 0,16384
  OwnBlitter_
  BTBlockImage 0,img,32,32,150
  WaitBlit_
  DisownBlitter_
  !CheckEq{Point(40,40),1,"pixel"}
  FreeMem_ m,80000
!EndTestCase
