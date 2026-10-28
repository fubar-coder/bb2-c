# The logic of the host tools as libraries (the tools are thin main.c
# around them; the tests use them directly):
#   hunk       reading AmigaDOS load files
#   blitzlib   Blitz Basic 2 library files: header, token table, checks
#   biglib     BigLib files (acidlibs, deflibs): writing and reading
#   hunkcheck  checking bytes at offsets of the first hunk
#   amiga_emulation  running AmigaDOS scripts in Amiberry for tests
#   vasmlisting  reading vasm listings (rows, symbols)
#   sha256     SHA-256 (hashes of generated test data)
add_subdirectory(hunk)
add_subdirectory(blitzlib)
add_subdirectory(biglib)
add_subdirectory(hunkcheck)
add_subdirectory(amiga_emulation)
add_subdirectory(vasmlisting)
add_subdirectory(sha256)
