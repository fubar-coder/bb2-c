; shapetrixlib-scale-plane-64k (#1047) - a shape keeps its plane sizes
; in words (+22 bytes per plane, +24 with the extra word of every row);
; Scale makes a new shape of the scaled size without checking it: a
; plane of 64K or more gets wrong word sizes.
; Here with runtime error checking: a shape of 256 x 500 pixels, Scale
; 0,2,2 (512 x 1000: 64 bytes per row, a plane of 64000 bytes, 66000
; with the extra word).
; Correct: an error check refuses the new shape: "Shape too Big", the
; command is not done.
; The error is announced with !ExpectError; the original raises none (see
; the configuration).
DEFTYPE .l

!TestCase{scale_plane_64k}
  InitShape 0,256,500,1
  !ExpectError{"Shape too Big","refused"}
  Scale 0,2,2
  !Check{0,"after"}
!EndTestCase
