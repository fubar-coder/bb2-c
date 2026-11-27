; byte-fraction-bitop - a constant with a fraction and a bit operator under
; a Byte type so far: the whole part, as of a variable (rule P+ of #1216):
; 1 AND 2; AmiBlitz3 v3.14.0 refuses it (#1188), with its own message
; (byte-fraction-bitop.test.toml)
!TestCase{byte_fraction_bitop}
  vb.b=2 : rb.b=0
  rb=1.5 AND vb
  !CheckEq{rb,0,"l1"}
!EndTestCase
