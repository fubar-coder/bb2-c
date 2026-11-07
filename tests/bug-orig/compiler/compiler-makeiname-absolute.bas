; compiler-makeiname-absolute (#341) - makeiname puts the path of IncDir
; in front of every file name of Include, XInclude and IncBin, also of a
; name with a device (':'): after IncDir "CORPUS:sub/", Include
; "CORPUS:src/inc.bb2" opens "CORPUS:sub/CORPUS:src/inc.bb2" and fails
; (Can't open Include). Found with the RWE libraries: LIBMACS.BB includes
; "blitzlibs:libnums.bb", a source with an IncDir cannot include LIBMACS.BB.
; Correct: a name with ':' is used as it is, the program prints 7.
; @fixed #341
; @pass file=src/inc.bb2|@TESTDATA@/compiler-makeiname-absolute.inc
IncDir "CORPUS:sub/"
Include "CORPUS:src/inc.bb2"
NPrint #k
End
