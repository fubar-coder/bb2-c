# ctest xtra_tool: the tool xtra on files in WORK - an AmiBlitz3 header
# checked byte-exact, converted to a Blitz 2 .xtra, checked, dumped and
# converted back; broken inputs refused with their message and exit 1,
# wrong arguments exit 2.
#   cmake -DXTRA=... -DWORK=... -P tooltest.cmake
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK})

# run xtra ARGS; expect exit status RC and (if given) MATCH in stdout+stderr
function(run rc match)
	execute_process(COMMAND ${XTRA} ${ARGN} RESULT_VARIABLE got OUTPUT_VARIABLE out ERROR_VARIABLE err)
	if(NOT got EQUAL rc)
		message(FATAL_ERROR "xtra ${ARGN}: exit ${got}, not ${rc}\n${out}${err}")
	endif()
	if(match AND NOT "${out}${err}" MATCHES "${match}")
		message(FATAL_ERROR "xtra ${ARGN}: no \"${match}\" in\n${out}${err}")
	endif()
	set(out "${out}" PARENT_SCOPE)
endfunction()

set(header "; XTRA
; Embedded .xtra Header
;
; General Info
; -------------------------------------------------------
; ExePath         = \"ram:\"
; ExeFile         = \"test\"
; CreateIcon      = 0
; Residents       = \"blitzlibs:amigalibs.res,libmacs.res\"
;
; Compiler
; -------------------------------------------------------
; StringBuffer    = 20000
; MakeSmallest    = 1
; FuncOptimize    = 1
; Version         = 1.2.3
; NumberOfBuilds  = 4
;
; Debugger
; -------------------------------------------------------
; CliArgs         = \"-v file\"
; StackSize       = 8191
; RuntimeDebug    = 1
; DebugInfo       = 0
; CreateDbgFile   = 0
; OverflowCheck   = 1
; AssemblerCheck  = 0
; InterruptCheck  = 1
; AutoRun         = 1
;
; Editor
; -------------------------------------------------------
; CursorLine      = 3
; CursorColumn    = 7
; LabelSearch     = \"\"
; LabelRemark     = 0
; LabelAll        = 0
; LabelCase       = 0
; LabelPosition   = 0
;
; Blitz Objects
; -------------------------------------------------------
; Max Screen      = 8
; /XTRA
")
# AmiBlitz3 writes the empty comment lines as "; " (a blank kept here)
string(REPLACE "\n;\n" "\n; \n" header "${header}")
file(WRITE ${WORK}/good.ab3 "${header}NPrint \"hello\"\nEnd\n")
run(0 "1 file ok" check --ab3 --exact ${WORK}/good.ab3)
run(0 "Screen\": 8" dump --ab3 ${WORK}/good.ab3)
run(0 "maximum \"Screen\" = 8 left out" convert --to-xtra ${WORK}/good.ab3 ${WORK}/good.xtra)
file(SIZE ${WORK}/good.xtra size)
if(NOT size EQUAL 1110)
	message(FATAL_ERROR "good.xtra: ${size} bytes, not 1110")
endif()
run(0 "1 file ok" check ${WORK}/good.xtra)
run(0 "resident 1 +\"libmacs.res\"" dump ${WORK}/good.xtra)
run(0 "" convert --to-ab3 ${WORK}/good.xtra ${WORK}/back.ab3)
file(READ ${WORK}/back.ab3 back)
foreach(line "; ExePath         = \"ram:\"" "; Residents       = \"blitzlibs:amigalibs.res,libmacs.res\""
		"; StringBuffer    = 20000" "; CliArgs         = \"-v file\"" "; MakeSmallest    = 1")
	string(FIND "${back}" "${line}\n" at)
	if(at LESS 0)
		message(FATAL_ERROR "back.ab3: no line ${line}")
	endif()
endforeach()
run(0 "1 file ok" check --ab3 --exact ${WORK}/back.ab3)

# broken inputs
file(WRITE ${WORK}/quote.ab3 "; XTRA\n; ExeFile = \"test\n; /XTRA\n")
run(1 "quote.ab3: line 2: string without its closing" check --ab3 ${WORK}/quote.ab3)
file(WRITE ${WORK}/noend.ab3 "; XTRA\n; ExeFile = \"test\"\n")
run(1 "noend.ab3: line 3: header without" dump --ab3 ${WORK}/noend.ab3)
run(1 "noend.ab3: line 3: header without" convert --to-xtra ${WORK}/noend.ab3 ${WORK}/no.xtra)
if(EXISTS ${WORK}/no.xtra)
	message(FATAL_ERROR "no.xtra written from a broken header")
endif()
file(WRITE ${WORK}/vers.xtra "hello")
run(1 "vers.xtra: offset 0: vers is neither 0 nor 1" check ${WORK}/vers.xtra)
string(ASCII 1 one)
file(WRITE ${WORK}/short.xtra "${one}")   # vers 1, 1 byte
run(1 "short.xtra: offset 1: file too short" check ${WORK}/good.xtra ${WORK}/short.xtra)
run(1 "short.xtra: offset 1: file too short" dump ${WORK}/good.xtra --bb22 ${WORK}/short.xtra)
run(1 "cannot read" check ${WORK}/missing.xtra)
run(1 "is not a BB22|not a BB22" check --bb22 ${WORK}/good.ab3)
run(2 "^usage: " check)
run(2 "^usage: " dump)
run(2 "^usage: " convert --to-xtra ${WORK}/good.ab3)
run(2 "^usage: " frobnicate x y)
message(STATUS "xtra: conversions and broken inputs as expected")
