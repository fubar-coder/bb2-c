# xtra - the companion settings of a source as one structure (xtra.h),
# shared like shared/filereq: every user compiles xtra.c (and xtra_ab3.c)
# itself with its own options - the host tools (tools/lib/xtra), later
# blitz2 and TED.  No product uses it yet; this object is the m68k
# compile with the options of blitz2 and TED (-fbaserel), as C99 without
# extensions and freestanding, so the code stays usable there.
add_library(xtra_m68k OBJECT xtra.c xtra_ab3.c)
set_target_properties(xtra_m68k PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(xtra_m68k PRIVATE -fbaserel -fomit-frame-pointer -ffreestanding
	-Wall -Wextra -pedantic)
