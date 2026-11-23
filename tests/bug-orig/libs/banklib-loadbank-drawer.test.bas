; banklib-loadbank-drawer (#72) - LoadBank takes the size of the file from
; fileiolib's Exists, which is -1 for a drawer; with a memory type other
; than the bank's, the bank is then reallocated with $FFFFFFFF bytes:
; memlib's 8 byte header makes it AllocMem(7), which succeeds, BankLoc
; points right behind that block and BankSize is -1 (the Open of the
; drawer then fails).
; Here a bank of 100 bytes (memtype 0) is loaded from the drawer SYS:C
; with memtype 2.
; Correct: a drawer is no file to load, the bank stays as it is (100
; bytes at the same address).  The original gives the size -1 and
; another address.
DEFTYPE .l

!TestCase{drawer}
  InitBank 0,100,0
  a=BankLoc(0)
  LoadBank 0,"SYS:C",2
  s=BankSize(0)
  l=BankLoc(0)
  !CheckEq{s,100,"size"}
  !Check{l=a,"same_address"}
!EndTestCase
