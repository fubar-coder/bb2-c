; compiler-amigalib-a4a5 (#958) - an Amiga library call pops its
; parameters into their registers before it loads the library base
; (move.l off(a5),a6) and saves neither a4 nor a5: a parameter in a5 makes
; a6 garbage (the jsr jumps anywhere), and after the call a4/a5 hold the
; parameters instead of the bases of the local/global variables.
; Here the case starts a child (compiler-amigalib-a4a5.test/child.bas:
; the code of the driver would be where the garbage a6 runs into): in a
; procedure, workbench AddAppIconA_ (taglist in a4; its other parameters
; are global, SHARED) and the local l read after it; then graphics
; LockLayerRom_ and UnlockLayerRom_ (statements) and AttemptLockLayerRom_
; (function), the layer in a5, on a layer of layers.library on a bitmap of
; its own, each followed by a read of the global g.  (exec Supervisor_,
; also a5, is tested by compiler-supervisor-token, #960.)
; Correct: the child prints local 7, global 42 four times and locked -1.
DEFTYPE .l

!TestCase{child}
  *r.TLRun=!RunProgram{"@TESTDIR@/child","child"}
  !CheckEq{*r\rc,0,"rc"}
  n$=Chr$(10)
  g$="global 42"+n$
  !CheckStrEq{*r\printed,"local 7"+n$+g$+g$+g$+g$+"locked -1"+n$,"printed"}
!EndTestCase
