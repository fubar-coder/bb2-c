; macros.bas - the macro text of a Blitz test driver (spec
; testing/basic-test-interface). The runner puts it in front of the
; preprocessed text of a test source. A block is a subroutine the driver
; calls with Gosub; its opening macro refers with Dc.l to the label its
; closing macro defines, so a block without its closing macro does not
; compile. The check macros take the number K of the preprocessor first.
; Labels with __ belong to the driver.
Macro TestBatchInit
  Dc.l tc__init_end
  .tc__init
  If TLInit=0 Then Return
End Macro
Macro EndTestBatchInit
  TLBlockEnd : Return
  .tc__init_end
End Macro
Macro TestCase
  Dc.l tc_end_`@
  .tc_`1
  If TLCase("`1")=0 Then Return
End Macro
Macro EndTestCase
  TLBlockEnd : Return
  .tc_end_`@
End Macro
Macro TestBatchCleanup
  Dc.l tc__final_end
  .tc__final
  If TLFinal=0 Then Return
End Macro
Macro EndTestBatchCleanup
  TLBlockEnd : Return
  .tc__final_end
End Macro
Macro Check
  TLCheck `1,-1,(`2)<>0
End Macro
Macro CheckEq
  TLCheck `1,`3,`2
End Macro
Macro CheckStrEq
  TLCheckStr `1,`3,`2
End Macro
Macro CheckQEq
  TLCheckQ `1,`3,`2
End Macro
Macro CheckFltNear
  TLCheckFlt `1,`3,`2,`4
End Macro
Macro Require
  TLRequire `1,`2
End Macro
Macro ExpectError
  TLExpectError `1
End Macro
; The result of a started program (the macro RunProgram, the routine
; TLRunProgram): one static block of the library that a later run
; overwrites; read only, its fields read through the pointer the macro
; gives (*r.TLRun=!RunProgram{K,c$} : x=*r\rc).
NEWTYPE .TLRun
  rc.l          ; the return code, 0 if unknown
  rcknown.w     ; -1: the system gave it; 0: it gave none (Kickstart 1.x)
  cut.w         ; -1: printed holds only the first 1024 bytes
  size.l        ; the bytes the program wrote
  printed.s     ; the first 1024 bytes of its output, as they are
End NEWTYPE
Macro RunProgram
TLRunProgram(`1,`2)
End Macro
