; byte-fraction-whole - a whole-valued literal with a point under a Byte
; type so far (#1188): the port keeps it the whole number it is (vb+2.0 an
; add of 2, vb AND 2.0 compiles) and Data.b 1.5 gives 1, as the original;
; AmiBlitz3 v3.14.0 makes every literal with a point a Float there, as
; under a Word type so far: it refuses vb AND 2.0 and Data.b 1.5
; (byte-fraction-whole.test.toml). The Data stands behind the case, as in
; the other tests of AmiBlitz3's Restore.
!TestCase{byte_fraction_whole}
  vb.b=2 : rb.b=0
  rb=vb+2.0
  !CheckEq{rb,4,"l1"}
  rb=vb AND 2.0
  !CheckEq{rb,2,"l2"}
  Restore bfw_d : Read rb
  !CheckEq{rb,1,"l3"}
!EndTestCase
.bfw_d
Data.b 1.5
