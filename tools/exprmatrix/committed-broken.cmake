# committed-broken.cmake - the broken-input test of exprmatrix_committed
# (CMakeLists.txt here): a copy of the matrix's committed files (the
# description, the measured values, a corpus source, its expected file, a
# bug-orig test) in WORK, checked as a tree by exprmatrix expect --check:
# the copy lacks the other files, and the edited source and an extra file
# must be named.
#   cmake -DEXPRMATRIX=exprmatrix -DREPO=DIR -DWORK=DIR -P committed-broken.cmake
foreach(v EXPRMATRIX REPO WORK)
	if(NOT DEFINED ${v})
		message(FATAL_ERROR "committed-broken: -D${v}=... missing")
	endif()
endforeach()
set(src tests/corpus/tests-exprmatrix)
file(MAKE_DIRECTORY ${WORK}/${src} ${WORK}/tests/corpus/expect/corpus_exprmatrix)
file(COPY ${REPO}/${src}/matrix.txt ${REPO}/${src}/measured.tsv ${REPO}/${src}/dl-A-agree-01.bas
	DESTINATION ${WORK}/${src})
file(COPY ${REPO}/tests/corpus/expect/corpus_exprmatrix/dl-A-agree-01.out
	DESTINATION ${WORK}/tests/corpus/expect/corpus_exprmatrix)
# a source edited by hand and a file the generator does not write
file(APPEND ${WORK}/${src}/dl-A-agree-01.bas "NPrint \"edited\"\n")
file(WRITE ${WORK}/tests/corpus/expect/corpus_exprmatrix/zz-extra-01.out "rc=0\n")
execute_process(COMMAND ${EXPRMATRIX} expect ${WORK}/${src}/matrix.txt ${WORK}/${src}/measured.tsv
	-o ${WORK} --check RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
set(bad "")
if(NOT rc EQUAL 1)
	string(APPEND bad "exit status ${rc}, wanted 1\n")
endif()
foreach(want
		"tests/corpus/tests-exprmatrix/dl-A-agree-01.bas: differs from the generator's output"
		"tests/corpus/expect/corpus_exprmatrix/zz-extra-01.out: not written by the generator"
		"tests/corpus/tests-exprmatrix/dl-A-agree-02.bas: missing")
	string(FIND "${out}" "${want}" at)
	if(at LESS 0)
		string(APPEND bad "no line \"${want}\"\n")
	endif()
endforeach()
string(FIND "${out}" "corpus_exprmatrix/dl-A-agree-01.out:" at)
if(NOT at LESS 0)
	string(APPEND bad "the unchanged dl-A-agree-01.out named\n")
endif()
if(bad)
	message(FATAL_ERROR "committed-broken:\n${bad}output:\n${out}${err}")
endif()
message("committed-broken: as expected")
