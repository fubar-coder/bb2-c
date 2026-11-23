; rimisclib-openfontprefs-second-leak (#757) - OpenFontPrefs allocates a
; new 16 KB buffer (AllocMem, not memlib: not freed at the end of the
; program) and stores it without freeing the one a call before left: a
; second OpenFontPrefs without CloseFontPrefs in between loses the first
; buffer until the next reboot.  The program opens the font prefs of the
; emulated system twice, closes them once and compares the free memory
; before and after.
; Correct: both OpenFontPrefs find the prefs (-1), nothing is lost.
DEFTYPE .l

!TestCase{second_open}
  m0=AvailMem_(0)
  x.w=OpenFontPrefs
  y.w=OpenFontPrefs
  CloseFontPrefs
  m1=AvailMem_(0)
  !CheckEq{x,-1,"first_open"}
  !CheckEq{y,-1,"second_open"}
  !Check{m0-m1<16384,"nothing_lost"}
!EndTestCase
