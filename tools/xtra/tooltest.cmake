# ctest xtra_tool: the tool xtra on files in WORK - an AmiBlitz3 header
# checked byte-exact, converted to a Blitz 2 .xtra, checked, dumped and
# converted back (as new header and into the source's header); sources
# with their type detected and their companion; broken inputs refused
# with their message and exit 1, wrong arguments exit 2.
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

# the .xtra into the source's own header: the same file (good.xtra came
# from it); another ExePath changes that line only; a source without
# header gets a new one
run(0 "" convert --to-ab3 ${WORK}/good.xtra ${WORK}/merged.ab3 --ab3 ${WORK}/good.ab3)
file(READ ${WORK}/good.ab3 good)
file(READ ${WORK}/merged.ab3 merged)
if(NOT merged STREQUAL good)
	message(FATAL_ERROR "merged.ab3 differs from good.ab3:\n${merged}")
endif()
string(REPLACE "\"ram:\"" "\"work:bin\"" other "${good}")
file(WRITE ${WORK}/other.ab3 "${other}")
run(0 "" convert --to-xtra ${WORK}/other.ab3 ${WORK}/other.xtra)
run(0 "" convert --to-ab3 ${WORK}/other.xtra ${WORK}/merged.ab3 --ab3 ${WORK}/good.ab3)
file(READ ${WORK}/merged.ab3 merged)
if(NOT merged STREQUAL other)
	message(FATAL_ERROR "merged.ab3 is not good.ab3 with ExePath work:bin:\n${merged}")
endif()
file(WRITE ${WORK}/plain.ab3 "NPrint 1\n")
run(0 "" convert --to-ab3 ${WORK}/good.xtra ${WORK}/merged.ab3 --ab3 ${WORK}/plain.ab3)
file(READ ${WORK}/merged.ab3 merged)
if(NOT merged MATCHES "^; XTRA\n.*\n; ExePath         = \"ram:\"\n.*\n; /XTRA\nNPrint 1\n$")
	message(FATAL_ERROR "merged.ab3: not a new header in front of plain.ab3:\n${merged}")
endif()

# sources: the type detected, the companion NAME.xtra read if there
file(COPY_FILE ${WORK}/good.xtra ${WORK}/prog.bb2)    # has $00 bytes: TED's
run(0 "companion +.*/prog.bb2.xtra \\(missing\\)\ntype +TED tokenised\n" dump --source ${WORK}/prog.bb2)
file(COPY_FILE ${WORK}/good.xtra ${WORK}/prog.bb2.xtra)
run(0 "companion +.*/prog.bb2.xtra\ntype +TED tokenised\nsource +Blitz 2 .xtra\n" dump --source ${WORK}/prog.bb2)
run(0 "type +AmiBlitz3 text\nsource header +[0-9]+ bytes kept\n" dump --source ${WORK}/good.ab3)
run(0 "type +AmiBlitz3 text\nsource +AmiBlitz3 embedded header\n" dump --source ${WORK}/plain.ab3)
file(WRITE ${WORK}/plain.bb2 "NPrint 1\n")
run(0 "\\(missing\\)\ntype +text without header\n" dump --source ${WORK}/plain.bb2)

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
run(1 "cannot read" dump --source ${WORK}/missing.bb2)
file(COPY_FILE ${WORK}/short.xtra ${WORK}/prog.bb2.xtra)
run(1 "prog.bb2.xtra: offset 1: file too short" dump --source ${WORK}/prog.bb2)
run(1 "noend.ab3: line 3: header without" convert --to-ab3 ${WORK}/good.xtra ${WORK}/no.ab3 --ab3 ${WORK}/noend.ab3)
run(1 "short.xtra: offset 1: file too short" convert --to-ab3 ${WORK}/short.xtra ${WORK}/no.ab3 --ab3 ${WORK}/good.ab3)
if(EXISTS ${WORK}/no.ab3)
	message(FATAL_ERROR "no.ab3 written from a broken input")
endif()
run(1 "is not a BB22|not a BB22" check --bb22 ${WORK}/good.ab3)
run(2 "^usage: " check)
run(2 "^usage: " dump)
run(2 "^usage: " convert --to-xtra ${WORK}/good.ab3)
run(2 "^usage: " frobnicate x y)
message(STATUS "xtra: conversions and broken inputs as expected")
