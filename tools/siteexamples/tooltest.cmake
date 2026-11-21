# ctest siteexamples_tool: siteexamples on fake site trees - the check and
# the written sources, and what it must refuse: a block without an entry, an
# entry without a block (the example went away or changed its number), the
# same block listed twice, a block without its closing fence, malformed
# manifests, an unreadable manifest or site.
#   cmake -DSITEEXAMPLES=... -DWORK=DIR -P tooltest.cmake
file(REMOVE_RECURSE ${WORK})
file(MAKE_DIRECTORY ${WORK}/site/content/docs ${WORK}/site/data/libintros)
file(WRITE ${WORK}/site/content/docs/a.md
	"# Title\n\n```blitzbasic\nNPrint \"a\"\n```\n\n```python\nprint(1)\n```\n\n```blitzbasic\nx=1\n```\n")
file(WRITE ${WORK}/site/data/libintros/b.yaml
	"description: x\nintro: |\n  Text.\n\n  ```blitzbasic\n  For i=1 To 3\n    NPrint i\n  Next\n  ```\n")
set(site ${WORK}/site)
set(manifest "# the examples\na-1 run content/docs/a.md#1\n\na-2 compile content/docs/a.md#2 opens a screen\nb-1 run data/libintros/b.yaml#1\n")
file(WRITE ${WORK}/ok.txt "${manifest}")

function(run expect_rc)
	execute_process(COMMAND ${ARGN} RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
	if(NOT rc EQUAL expect_rc)
		message(FATAL_ERROR "${ARGN}: exit ${rc}, expected ${expect_rc}: ${out}${err}")
	endif()
	set(out "${out}" PARENT_SCOPE)
	set(err "${err}" PARENT_SCOPE)
endfunction()

# refused(TEXT REGEX): the manifest TEXT fails the check with a message matching REGEX
function(refused text regex)
	file(WRITE ${WORK}/bad.txt "${text}")
	run(1 ${SITEEXAMPLES} check ${site} ${WORK}/bad.txt)
	if(NOT err MATCHES "${regex}")
		message(FATAL_ERROR "manifest:\n${text}\nexpected ${regex}, got: ${err}")
	endif()
endfunction()

# the check: counts, nothing else on stdout
run(0 ${SITEEXAMPLES} check ${site} ${WORK}/ok.txt)
if(NOT out STREQUAL "siteexamples: 3 examples, 2 run, 1 compiled only\n")
	message(FATAL_ERROR "check: ${out}")
endif()

# extract: a source per entry, the indentation of the YAML block taken off,
# the origin in the first line, only the blitzbasic blocks
run(0 ${SITEEXAMPLES} extract ${site} ${WORK}/ok.txt ${WORK}/out)
file(READ ${WORK}/out/a-1.bas got)
if(NOT got STREQUAL "; content/docs/a.md#1 (run)\nNPrint \"a\"\n")
	message(FATAL_ERROR "a-1.bas:\n${got}")
endif()
file(READ ${WORK}/out/a-2.bas got)
if(NOT got STREQUAL "; content/docs/a.md#2 (compiled only)\nx=1\n")
	message(FATAL_ERROR "a-2.bas:\n${got}")
endif()
file(READ ${WORK}/out/b-1.bas got)
if(NOT got STREQUAL "; data/libintros/b.yaml#1 (run)\nFor i=1 To 3\n  NPrint i\nNext\n")
	message(FATAL_ERROR "b-1.bas:\n${got}")
endif()
file(GLOB written ${WORK}/out/*.bas)
list(LENGTH written n)
if(NOT n EQUAL 3)
	message(FATAL_ERROR "${n} sources written: ${written}")
endif()

# a block without an entry: a new example is not skipped silently
refused("a-1 run content/docs/a.md#1\nb-1 run data/libintros/b.yaml#1\n"
	"content/docs/a.md#2 \\(line 11\\): the example has no entry in the manifest")
# an entry without a block (the number is past the blocks of the file, or
# the file has none)
refused("${manifest}a-3 run content/docs/a.md#3\n"
	"manifest line 6: a-3: the site has no block content/docs/a.md#3")
refused("${manifest}c-1 run content/docs/c.md#1\n"
	"manifest line 6: c-1: the site has no block content/docs/c.md#1")
# the same block twice
refused("${manifest}a-1b run content/docs/a.md#1\n"
	"manifest line 6: content/docs/a.md#1 is already listed in line 2")
# the manifest itself
refused("${manifest}a-1 run content/docs/a.md#1\n" "line 6: the name a-1 is used in line 2")
refused("A-1 run content/docs/a.md#1\n" "line 1: the name must be lower case")
refused("a-1 skip content/docs/a.md#1\n" "line 1: the mode is run or compile")
refused("a-1 compile content/docs/a.md#1\n" "line 1: a compile entry needs the reason")
refused("a-1 run content/docs/a.md#1 why\n" "line 1: a run entry has no reason")
refused("a-1 run content/docs/a.md\n" "line 1: FILE#N expected")
refused("a-1 run content/docs/a.md#0\n" "line 1: the block number")
refused("a-1 run content/docs/a.md#x\n" "line 1: the block number")
refused("a-1 run\n" "line 1: NAME MODE FILE#N")

# a block without its closing fence
file(WRITE ${WORK}/site/content/docs/open.md "text\n\n```blitzbasic\nx=1\n")
run(1 ${SITEEXAMPLES} check ${site} ${WORK}/ok.txt)
if(NOT err MATCHES "content/docs/open.md: line 3: the block has no closing fence")
	message(FATAL_ERROR "open block: ${err}")
endif()
file(REMOVE ${WORK}/site/content/docs/open.md)

# a site that is no directory, a manifest that is missing, nothing written
# on a failed check
run(1 ${SITEEXAMPLES} check ${WORK}/nosite ${WORK}/ok.txt)
if(NOT err MATCHES "nosite: not a directory")
	message(FATAL_ERROR "no site: ${err}")
endif()
run(1 ${SITEEXAMPLES} check ${site} ${WORK}/missing.txt)
if(NOT err MATCHES "cannot read .*missing.txt")
	message(FATAL_ERROR "no manifest: ${err}")
endif()
file(WRITE ${WORK}/bad.txt "a-1 run content/docs/a.md#1\n")
run(1 ${SITEEXAMPLES} extract ${site} ${WORK}/bad.txt ${WORK}/out2)
if(EXISTS ${WORK}/out2)
	message(FATAL_ERROR "a failed extract wrote ${WORK}/out2")
endif()
# usage errors
run(2 ${SITEEXAMPLES} check ${site})
run(2 ${SITEEXAMPLES})
message(STATUS "siteexamples_tool: all as expected")
