# page-broken.cmake - the broken-input test of the check of the page
# docs/blitz2/expression-type-tables.md (CMakeLists.txt here): two copies of
# the committed page in WORK, one with a changed cell (the first "W W → L"
# made "W W → W") and one without the note of #1209, each checked by
# exprtables page --check -o OUT: the check must fail (exit status 1), name
# the changed line, and leave no OUT (one there from an earlier run is
# removed).
#   cmake -DEXPRTABLES=exprtables -DPAGE=page.md -DWORK=DIR -P page-broken.cmake
foreach(v EXPRTABLES PAGE WORK)
	if(NOT DEFINED ${v})
		message(FATAL_ERROR "page-broken: -D${v}=... missing")
	endif()
endforeach()
file(MAKE_DIRECTORY ${WORK})
file(READ ${PAGE} page)
set(bad "")
set(out "")
set(err "")

# checks the copy WORK/NAME.md, whose change is at line LINE
function(check name line)
	file(WRITE ${WORK}/${name}-out.md "an earlier page\n")
	execute_process(COMMAND ${EXPRTABLES} page --check ${WORK}/${name}.md -o ${WORK}/${name}-out.md
		RESULT_VARIABLE rc OUTPUT_VARIABLE o ERROR_VARIABLE e)
	if(NOT rc EQUAL 1)
		string(APPEND bad "${name}: exit status ${rc}, wanted 1\n")
	endif()
	string(FIND "${e}" "${name}.md:${line}: differs from the generated page" found)
	if(found LESS 0)
		string(APPEND bad "${name}: line ${line} not named\n")
	endif()
	if(EXISTS ${WORK}/${name}-out.md)
		string(APPEND bad "${name}: the page of the earlier run is still there\n")
	endif()
	set(bad "${bad}" PARENT_SCOPE)
	set(out "${out}${o}" PARENT_SCOPE)
	set(err "${err}${e}" PARENT_SCOPE)
endfunction()

# the line number of the offset AT of the page
function(lineof at var)
	string(SUBSTRING "${page}" 0 ${at} head)
	string(REGEX MATCHALL "\n" nl "${head}")
	list(LENGTH nl n)
	math(EXPR n "${n} + 1")
	set(${var} ${n} PARENT_SCOPE)
endfunction()

# a changed cell
set(cell "| W W → L |")
string(FIND "${page}" "${cell}" at)
if(at LESS 0)
	string(APPEND bad "no cell \"${cell}\" in ${PAGE}\n")
else()
	string(LENGTH "${cell}" n)
	math(EXPR after "${at} + ${n}")
	string(SUBSTRING "${page}" 0 ${at} head)
	string(SUBSTRING "${page}" ${after} -1 tail)
	file(WRITE ${WORK}/cell.md "${head}| W W → W |${tail}")
	lineof(${at} line)
	check(cell ${line})
	string(FIND "${err}" "cell.md: lines that differ from the generated page: 1" found)
	if(found LESS 0)
		string(APPEND bad "cell: not exactly one line named\n")
	endif()
endif()

# a missing note: the line of #1209 in the notes of the bit operators
string(FIND "${page}" "Under a Quick type so far (#1209)" at)
if(at LESS 0)
	string(APPEND bad "no note of #1209 in ${PAGE}\n")
else()
	string(SUBSTRING "${page}" ${at} -1 from)
	string(FIND "${from}" "\n" eol)
	math(EXPR eol "${at} + ${eol} + 1")
	string(SUBSTRING "${page}" 0 ${at} head)
	string(SUBSTRING "${page}" ${eol} -1 tail)
	file(WRITE ${WORK}/note.md "${head}${tail}")
	lineof(${at} line)
	check(note ${line})
endif()

if(bad)
	message(FATAL_ERROR "page-broken:\n${bad}output:\n${out}${err}")
endif()
message("page-broken: as expected")
