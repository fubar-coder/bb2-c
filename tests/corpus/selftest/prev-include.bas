; prev-include - the source of the self tests of the comparison with prev
; (compiler_selftest_*_prev in tests/corpus/CMakeLists.txt): it includes
; prev-include.inc, which is the same file for both compilers or another one
Include "CORPUS:src/prev-include.inc"
NPrint Asc(i$)," ",Asc(Right$(i$,1))," ",Len(i$)
End
