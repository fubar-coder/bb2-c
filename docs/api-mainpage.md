# API reference {#mainpage}

The C side of the BB2 C port's libraries, made by Doxygen from the
comments in the sources (`Doxyfile` in the repository root, built by the
website's Pages workflow). It is for developers who write or change a
library in C; the commands as a Blitz Basic programmer sees them are in
the command reference of the website:
<https://fubar-coder.github.io/bb2-c/docs/libraries/>.

What it covers:

- **The library headers** (`libs/include`): `blitzlib.h` with the macros
  that write a library's header and declaration table, the calling
  conventions (register parameters, a3/a4/a5) and the thunks;
  `blitzhelp.h`, `blitzexec.h`, `blitzobj.h` (the objects of the
  libraries), `deflib.h` of the AmigaOS call libraries and `optidec.h`
  of the RWE userlibs.
- **The command libraries** (`libs/acidlibs`) and **the RWE userlibs**
  (`libs/userlibs/rwe`): every routine with its comment - what the
  command does, its arguments with their registers, its result, and the
  notes on original bugs (`BUG(orig)` with the issue number).
- **The shared code** (`shared/`): the parts compiled into several
  programs, such as the file requester, the `.xtra` files and the
  tokenizer.

Conventions of the comments (`docs/development.md` in the repository):
the brief of a routine that implements a Blitz command starts with the
command's name; `@param` and `@return` name the register (`d0: ...`).
Routines written in assembler have no C parameters: their register
arguments are listed as items and their result in a "Result" paragraph.
