; windowslib-windowtags-pairs-limit (#961) - WindowTags
; Window#,Flags,Title$,Tag,Data[,Tag,Data...] copies its tag pairs from
; the argument stack into the library's buffer of 16 user tags without a
; limit, then a TAG_DONE: 16 pairs or more write behind the buffer (in
; the original over bmapbase, from 17 pairs on over the code of
; WindowFont). The pairs lie on the argument stack in reverse order, so
; they cannot be linked with TAG_MORE as the &TagList form does (#317).
; An invalid argument: the error check catches it (BB2 style); the
; command itself is unchanged. Runtime error checking on (the debugger);
; WindowTags with 16 pairs (WA_Left, harmless).
; Correct: the error check stops it with "Too Many Tags".  The original
; has no check and opens the window.
DEFTYPE .l

!TestCase{sixteen_pairs}
  WbToScreen 0
  t=$80000064
  !ExpectError{"Too Many Tags","refused"}
  WindowTags 0,$000E,"Tags",t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10
  !Check{0,"not_refused"}
  CloseWindow 0
!EndTestCase
