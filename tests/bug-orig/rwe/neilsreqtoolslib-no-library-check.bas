; neilsreqtoolslib-no-library-check (#463) - init sets IsReqToolsActive
; to 0 when reqtools.library V38 does not open, but no command tests it
; (or the base): RTEZRequest loads the base 0 into a6 and jumps through
; its vector table.  The program runs through inject with the open
; failing (runwrap=, inject openlibfail=reqtools.library): the jump to a
; negative offset from 0 crashes the system (the session ends in a
; timeout).
; Correct: RTEZRequest returns 0 without reqtools, the program goes on.
; @rwe neilsreqtoolslib
; @pass runwrap=@INJECT@|openlibfail=reqtools.library
NPrint IsReqToolsActive
r.l=RTEZRequest("BugOrig","Hello","Ok")
NPrint r
End
