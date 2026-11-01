# tokenizer - source text to tokenised lines (tokenizer.h), shared like
# shared/xtra: every user compiles tokenizer.c itself with its own options
# - ab3cli (TED's options), the host tools (tools/lib/tokenizer).  This
# object is the m68k compile with the options of blitz2 and TED
# (-fbaserel), as C99 without extensions and freestanding, so the code
# stays usable there.
add_library(tokenizer_m68k OBJECT tokenizer.c)
set_target_properties(tokenizer_m68k PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(tokenizer_m68k PRIVATE -fbaserel -fomit-frame-pointer -ffreestanding
	-Wall -Wextra -pedantic)
