; neilsreqtoolslib-no-library-check (#463) - init sets IsReqToolsActive
; to 0 when reqtools.library V38 does not open, but no command tests it
; (or the base): RTEZRequest loads the base 0 into a6 and jumps through
; its vector table.  The program runs through inject with the open
; failing (runwrap=, inject openlibfail=reqtools.library): the jump to a
; negative offset from 0 crashes the system (the session ends in a
; timeout).
; Correct: without reqtools every command returns what it returns on
; cancel and the program goes on: RTEZRequest 0, RTEZLoadFile an empty
; string (in a string expression too), RTEZGetLong with a default the
; default, RTEZSetPattern nothing, RTVersion 0.
DEFTYPE .l

!TestCase{no_reqtools}
  !CheckEq{IsReqToolsActive,0,"active"}
  r=RTEZRequest("BugOrig","Hello","Ok")
  !CheckEq{r,0,"request"}
  !CheckStrEq{"<"+RTEZLoadFile("Load","file")+">","<>","load_file"}
  !CheckEq{RTEZGetLong("Number","Enter it",7),7,"get_long"}
  RTEZSetPattern 0,"#?.bb2"
  !CheckEq{RTVersion,0,"version"}
!EndTestCase
