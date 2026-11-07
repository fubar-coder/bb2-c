---
title: Fixed original bugs
weight: 1
description: The bugs of Blitz Basic 2 that the BB2 C port fixes deliberately, and which programs notice the difference.
prev: /docs/compared-with-original
---

Before this page you should know what the chapter means by a difference
([Differences from the original]({{< relref "/docs/compared-with-original" >}})).

The bugs of the original are fixed one at a time, area by area; this page
lists the ones fixed so far. Each fix changes only the case that was
wrong; a program that never ran into the bug behaves exactly as before,
and in most cases is compiled to exactly the same executable. Every entry
links its issue, which describes the bug in detail and names the test
that checks the correct behaviour. Nearly all of these tests run with the normal
emulator tests, so a fix that is undone later is noticed. The bugs that
are not fixed yet are on their own page
([Bugs not fixed yet]({{< relref "/docs/compared-with-original/bugs-not-fixed-yet" >}})).

## The compiler

These fixes change what the compiler does or the code it generates. They
are grouped by what blitz2 is doing when the bug strikes.

### Compiling

| Effect of the bug in the original | With the port |
|---|---|
| [Large executables did not load on AmigaOS 2.0 and later (#943)](https://github.com/fubar-coder/bb2-c/issues/943): with very many relocations the executable's relocation table had a form newer AmigaOS versions refuse. | The table is split into groups AmigaOS accepts. Only such very large programs are written differently. |
| [Calls of AmigaOS functions with an argument in register a4 or a5 crashed (#958)](https://github.com/fubar-coder/bb2-c/issues/958), or destroyed the program's variables, because those registers hold the variable bases. | The compiler saves a4 and a5 around such a call. Only these few calls get different code. |
| [A string addition from the seventh argument of a library command on got a wrong length (#971)](https://github.com/fubar-coder/bb2-c/issues/971), for example the text of a window; also a string addition nested that deeply. | The length is right. |
| [Pad bytes contained random values (#969)](https://github.com/fubar-coder/bb2-c/issues/969): the bytes skipped by `Even`, by the alignment of data and code, and the space reserved with `Ds` held whatever the compiler's memory held. | They are zero, so they no longer depend on the compiler's memory. |
| [A constant defined without a value held a random value (#972)](https://github.com/fubar-coder/bb2-c/issues/972): a line `#c` alone was accepted, and `#c` could be anything. | Its value is 0. |
| [A NewType that contained a field of its own type got a random size (#973)](https://github.com/fubar-coder/bb2-c/issues/973). | The compiler reports "Illegal Type"; a pointer to the own type stays allowed. |
| [A `~` inside a string literal started a constant expression (#966)](https://github.com/fubar-coder/bb2-c/issues/966): `"a~1+2~b"` became `a3b`, a single `~` in a string an error. | Inside strings `~` is an ordinary character. |
| [The Macro Buffer option had no effect, and blitz2 could crash when it ended (#336)](https://github.com/fubar-coder/bb2-c/issues/336) if a source had another Macro Buffer size. | The option sets the largest macro text, and the buffer is freed correctly. |
| [An error inside a macro definition could crash the compiler (#965)](https://github.com/fubar-coder/bb2-c/issues/965) instead of showing the error. | The error is shown. |
| [The last constant of a resident pointed behind the resident (#34)](https://github.com/fubar-coder/bb2-c/issues/34), so every compile freed memory that did not belong to it and could crash the system. | The list ends where it should. |
| [Resident files contained bytes nobody had set (#970)](https://github.com/fubar-coder/bb2-c/issues/970), so CREATE RESIDENT gave a different file each time. | Those bytes are zero; the same source gives the same resident. |
| [`IncBin` kept a file or directory in use after an error (#38)](https://github.com/fubar-coder/bb2-c/issues/38). | The file is closed; the errors are the same. |
| [On a 68010 processor the trap vectors were taken from the wrong table (#42)](https://github.com/fubar-coder/bb2-c/issues/42) when the system had moved them, which affects runtime errors and the debugger. | The moved table is used, on the 68010 as on later processors. |
| [The path of `IncDir` stayed for the following compiles (#342)](https://github.com/fubar-coder/bb2-c/issues/342): after a source with `IncDir`, also a resident compiled before, a source without `IncDir` compiled in the same session looked for its `Include`, `XInclude` and `IncBin` files in the other source's directory and failed with "Can't open Include". | Every compile starts without an `IncDir` path. |
| [After an `IncDir`, a file name with a device or assign could not be included (#341)](https://github.com/fubar-coder/bb2-c/issues/341): the `IncDir` path was put in front of it, so after `IncDir "BlitzLibs:"` the line `Include "blitzlibs:libnums.bb"` opened `BlitzLibs:blitzlibs:libnums.bb` and failed. | A name with a device or assign is used as written, for `Include`, `XInclude` and `IncBin`. AB3 mode still puts the `IncDir` path in front of every name. |
| [A jump to a label with the name of a constant of value 0 gave a nonsense error (#977)](https://github.com/fubar-coder/bb2-c/issues/977): with `#foo=0`, a `Goto`, `Gosub`, `Restore` or `?` to `foo` was taken as a label not defined yet, and the compile ended later with "Label reference out of context" at a line that does not exist, or could read and write random memory. | The compiler reports "Label has been used as a Constant" at the jump, as it does for a constant of any other value. |
| [A program ending with an `IncBin` of odd length crashed at its end (#979)](https://github.com/fubar-coder/bb2-c/issues/979): the code that ends the program started at an odd address, and `End` caused an address error. The same happens with a `Dc.b` of odd length at the end. | The end code starts at an even address and the program ends normally. Only such programs are compiled differently. |
| [A resident with a `NewType` without fields lost the definitions behind it (#35)](https://github.com/fubar-coder/bb2-c/issues/35): when the resident was loaded, the empty type was linked to a wrong place, and the types, macros and constants after it were lost or read from random memory. | The empty type is loaded as it was defined, and everything after it too. The resident file is the same. |
| [A one-line macro with a colon right after its name expanded to a text that started with a colon (#942)](https://github.com/fubar-coder/bb2-c/issues/942): `Macro a:1:End Macro` stored `:1`, so `NPrint !a` became `NPrint :1`, a syntax error. | The colon is skipped like a line end, and `!a` expands to `1`. The form with a space after the name, `Macro a 1:End Macro`, is unchanged. |
| [A macro line ending in `End` cut off the rest of the macro (#968)](https://github.com/fubar-coder/bb2-c/issues/968): the lines after it and the rest of the line that called the macro were not compiled, so an `End` inside a block `If` gave "If Without End If" at the call. In an included file, `End Macro` right after such a line gave "Syntax Error" at the `Include` line. This also happened with a comment or a `:` after the `End`. | The rest of the macro is kept, as in AmiBlitz3. |
| [A macro text of 65536 bytes or more was cut (#964)](https://github.com/fubar-coder/bb2-c/issues/964): the macro expanded to a shortened text, and part of its memory was lost until a reboot. Such a long text needs a Macro Buffer larger than 65535 bytes. | A macro text longer than 65535 bytes gives the error "Macro too Big". |
| [After the object buffer ran full, the compiler lost a little memory (#14)](https://github.com/fubar-coder/bb2-c/issues/14): when the buffer overflowed during an assignment or a `Select`, a buffer that holds code for a moment was not freed, once per such compile. This happens when a compile ends with "Not Enough Object Memory Available!" and is repeated with a larger Object Buffer. | The buffer is freed in every case. The generated code is the same. |
| [A constant definition without a name was accepted (#7)](https://github.com/fubar-coder/bb2-c/issues/7): `#=5` created a constant with an empty name, and a second such line made the compiler search 65536 bytes of memory for it. | "Syntax Error" at the line. |
| [A `#`, `Goto`, `Gosub`, `Restore`, `?` or `.` without a name made the compiler read far past the name (#30)](https://github.com/fubar-coder/bb2-c/issues/30): it searched its lists for a name of length 0 and compared 65536 bytes of memory each time. | "Syntax Error" at the line. |
| [A name of more than 127 characters overwrote the compiler's data (#29)](https://github.com/fubar-coder/bb2-c/issues/29): such a name, possible through a macro, was copied into a 128-byte buffer without a limit. | "Syntax Error" at the line. |
| [`Dcb` with a count of 0 made 2^32 copies (#37)](https://github.com/fubar-coder/bb2-c/issues/37): the compiler ran for hours and could overwrite memory. | A count of 0 emits nothing, a negative count gives "Illegal Constant". |
| [`For a$=… To …` with a global string variable compiled (#39)](https://github.com/fubar-coder/bb2-c/issues/39): the loop treated the string as a number. With a local string variable the compiler already refused it. | "Bad Type for For...Next", as for a local string variable. |
| [A long inline assembler expression lost memory (#1)](https://github.com/fubar-coder/bb2-c/issues/1): an expression of 240 characters or more in a macro or include was freed with a wrong size. | It is freed with its right size. The generated code is the same. |
| [A number ending in a decimal point was a syntax error (#10)](https://github.com/fubar-coder/bb2-c/issues/10): `a.f=1.` was refused. | `1.` is the float 1. |
| [An exponent after a fraction of 0 was ignored (#11)](https://github.com/fubar-coder/bb2-c/issues/11): `1.0e5` was read as 1.0 followed by "Garbage at End of Line". | `1.0e5` is 100000. |
| [An `IncDir` path of 96 characters or more overwrote the compiler's data (#16)](https://github.com/fubar-coder/bb2-c/issues/16): the program compiled and then crashed when it ran. | "Path too Long" at the `IncDir` line. |
| [An include file name of more than 127 characters overwrote the compiler's data (#325)](https://github.com/fubar-coder/bb2-c/issues/325): the `IncDir` path and the name of an `Include`, `XInclude` or `IncBin` were joined in a 128-byte buffer without a limit. | "Path too Long" at the line. |
| [A `UsePath` path of 128 characters or more overwrote the compiler's own code (#31)](https://github.com/fubar-coder/bb2-c/issues/31): the next `UsePath` then ran the damaged code. | "Path too Long" at the line. |
| [A compile error in some assignments or in `Select` lost a little memory (#994)](https://github.com/fubar-coder/bb2-c/issues/994): a buffer that holds code for a moment was not freed, once per such error. | The buffer is freed at the end of every pass. The generated code is the same. |
| [A resident with a long macro was broken (#995)](https://github.com/fubar-coder/bb2-c/issues/995): a macro text of 32767 bytes or more made the types and constants behind it load from the wrong place, and a text of exactly 65535 bytes was left out. Such long texts need a Macro Buffer over 32767 bytes. | The resident is written and loaded correctly. Only such residents are written differently. |

### Loading the libraries

At its start, and with RELOAD ALL LIBS, blitz2 loads the command libraries
from `blitz2:deflibs`, `blitz2:acidlibs` and the directories in
`blitz2:userlibs`.

| Effect of the bug in the original | With the port |
|---|---|
| [A read error while loading a library file went unnoticed (#3)](https://github.com/fubar-coder/bb2-c/issues/3): blitz2 went on with whatever the file read before had left in its buffer, which gave wrong library data or ended the session with an out-of-memory error. | A read error is noticed. A library whose header or structure cannot be read completely is left out; a read error in its relocation data ends the reading as at the end of a short file. |
| [When blitz2 could not examine `blitz2:deflibs` or `blitz2:acidlibs`, it loaded the file with a wrong length (#27)](https://github.com/fubar-coder/bb2-c/issues/27): it took the length of the file examined before, and loaded and linked a cut or overlong part of the file. | The file counts as missing, as if it were not there. |
| [A directory in `blitz2:userlibs` with a name of 32 characters or more overwrote memory (#28)](https://github.com/fubar-coder/bb2-c/issues/28): its name was copied into a 32-byte field without a limit. Such names occur on file systems with long names and on host directories mounted in an emulator. | Such a directory is skipped, and its libraries are not loaded. The name is not cut, because a cut name could name another directory. |
| [A library file whose relocation data could not be read was used anyway (#993)](https://github.com/fubar-coder/bb2-c/issues/993): blitz2 then patched the library at random places, which gave broken programs or corrupted memory. | The library is left out, as for a short header (#3). |

### Starting and ending blitz2

| Effect of the bug in the original | With the port |
|---|---|
| [Started from the Workbench, blitz2 left a short or damaged icon file open (#19)](https://github.com/fubar-coder/bb2-c/issues/19): it reads its `.info` file for the stack size and closed it only after a complete read, so the file stayed in use until the reboot. | The `.info` file is closed after any read. |
| [Started from the Workbench without a project icon, blitz2 could ask the editor to load its icon as a program (#20)](https://github.com/fubar-coder/bb2-c/issues/20): when its `.info` file could not be read, the editor got the name of the `.info` file, or a part of the icon's bytes, as the program to load. | Without a project icon the editor starts with no program. |
| [blitz2 always returned 0 to the shell (#22)](https://github.com/fubar-coder/bb2-c/issues/22), also when it gave up at the start, so a script could not tell a failed start from a normal session. | blitz2 returns -1 when it cannot start, for example without `mathffp.library` or without memory for its buffers, and 0 after a normal session. In AB3 mode, a compile from the command line keeps its own return codes ([Compiling from the command line]({{< relref "/docs/ab3-mode/command-line" >}})). |
| [Without chip memory for the arrow images of the options requester, blitz2 wrote to address 0 (#8)](https://github.com/fubar-coder/bb2-c/issues/8), into the lowest bytes of memory, which hold the pointer to the system's main library and the processor's exception vectors, and it freed address 0 when it ended. | blitz2 gives up at the start, with return code -1, as when other memory it needs at the start is missing. |
| [With 256 MB or more of free memory, blitz2 lost 256 MB when it ended (#26)](https://github.com/fubar-coder/bb2-c/issues/26): to make AmigaOS remove unused libraries from memory it asks for 256 MB, a request that is meant to fail. When the request succeeded, the memory was not freed until the reboot. | A successful request is freed again at once. |
| [Started from the Workbench, blitz2 did not go back to its start directory when it ended (#21)](https://github.com/fubar-coder/bb2-c/issues/21): it ended in its own directory, or in that of the project icon it was started with. | The start-up code restores the start directory when blitz2 ends. |
| [Started from the Workbench, blitz2 could crash at its very end (#337)](https://github.com/fubar-coder/bb2-c/issues/337): it let the Workbench unload it before it had returned, which goes wrong when the freed memory is reused at once or a memory checker such as MungWall overwrites it. | blitz2 has ended before the Workbench unloads it. |

### The editor's compiler menu

| Effect of the bug in the original | With the port |
|---|---|
| [LOAD DEBUG MODULE left the busy pointer on (#23)](https://github.com/fubar-coder/bb2-c/issues/23) when no debugger module had been loaded at the start, that is without `blitz2:defaultdbug`. | The mouse pointer returns to normal after the module is loaded. |
| [When RUN or COMPILE AND RUN could not start the program, the `EXECUTING` requester stayed open (#25)](https://github.com/fubar-coder/bb2-c/issues/25) with no program running, and the next run's requester did not work as it should. This happens when there is no memory for the program's process or its stack. | The requester is closed. As in the original, no error message is shown. |

## Runtime errors without the debugger

Every library has an area of error checks that is left out when a
program is compiled without runtime error checking. Several libraries had
error handling that jumped into that area, or kept the text of an error
message there. Without runtime error checking, such an error jumped to
memory behind the library, or reported a message that pointed into
nowhere. The port puts this code and these messages where they belong, so
the error is reported with its proper text in both cases. With runtime
error checking on, nothing changes.

The fixes concern:

* [memory errors, "Out of Memory!" (#956)](https://github.com/fubar-coder/bb2-c/issues/956)
* [fonts, "Can't Open Font" and others (#955)](https://github.com/fubar-coder/bb2-c/issues/955)
* [screens, "Can't open Screen" and others (#251)](https://github.com/fubar-coder/bb2-c/issues/251)
* [displays and copper lists (#412)](https://github.com/fubar-coder/bb2-c/issues/412)
* [shapes, `DecodeShapes` of address 0 (#270)](https://github.com/fubar-coder/bb2-c/issues/270) and [the shape error messages (#953)](https://github.com/fubar-coder/bb2-c/issues/953)
* [sprite files (#954)](https://github.com/fubar-coder/bb2-c/issues/954)
* [`SetPeriod` without a sound, and the sound error messages (#70)](https://github.com/fubar-coder/bb2-c/issues/70)
* [animations, "Can't Open Anim File" (#398)](https://github.com/fubar-coder/bb2-c/issues/398)
* [music modules, "Error Accessing Module File" (#957)](https://github.com/fubar-coder/bb2-c/issues/957)
* [IFF pictures (#192)](https://github.com/fubar-coder/bb2-c/issues/192) and [saving IFF files, "Can't open File" (#185)](https://github.com/fubar-coder/bb2-c/issues/185)
* [Workbench arguments without memory at the start of a program (#305)](https://github.com/fubar-coder/bb2-c/issues/305)

## Command libraries

| Effect of the bug in the original | With the port |
|---|---|
| [`Dim List` with size 0 wrote behind its memory block (#59)](https://github.com/fubar-coder/bb2-c/issues/59). | Nothing is written behind the block. |
| [`Date$` showed the 1st of January after a leap year as "32/12" (#120)](https://github.com/fubar-coder/bb2-c/issues/120), and the day, month and year functions were wrong for that day. | The date is the 1st of January. |
| [Characters far above the font's first character were not drawn (#148)](https://github.com/fubar-coder/bb2-c/issues/148), among them the umlauts in the standard font. | They are drawn. |
| [`GTPalette` with a colour ignored the colour and broke a following tag list (#168)](https://github.com/fubar-coder/bb2-c/issues/168). | The colour is set. |
| [GadTools gadgets took over optional values of an earlier gadget (#169)](https://github.com/fubar-coder/bb2-c/issues/169): a gadget created without, say, its start value got the previous gadget's. | Without the value, a gadget starts with the default. |
| [All `GTShape` gadgets showed the images of the last one (#173)](https://github.com/fubar-coder/bb2-c/issues/173), and GadTools' own image was overwritten. | Each gadget keeps its own images. |
| [`GTShape` wrote to low memory when the gadget could not be created (#327)](https://github.com/fubar-coder/bb2-c/issues/327). | A failed gadget is left alone. |
| [Cycle and MX gadgets had room for a limited number of labels without a check, and the memory was never freed (#174)](https://github.com/fubar-coder/bb2-c/issues/174). | The labels are counted, and the memory is freed with the gadget. |
| [Faded and decoded palettes were wrong or black on Kickstart before 3.0 (#224)](https://github.com/fubar-coder/bb2-c/issues/224). | The colours are right. |
| [A screen mode with a monitor part, such as PAL or NTSC, opened on the default monitor (#250)](https://github.com/fubar-coder/bb2-c/issues/250). | The whole mode is used. |
| [`WindowTags` with a long tag list overwrote code of the library (#317)](https://github.com/fubar-coder/bb2-c/issues/317). | A long list is copied only in part, the rest is linked. |
| [Window titles were copied and never freed (#318)](https://github.com/fubar-coder/bb2-c/issues/318): every window and every new title cost memory until the program ended. | The copies are freed with the window or the next title. |
| [`WMove` and `WSize` hung when Intuition limited the new position or size (#320)](https://github.com/fubar-coder/bb2-c/issues/320). | They wait only briefly. |
| [Closing one window discarded the events of all other windows (#322)](https://github.com/fubar-coder/bb2-c/issues/322). | Only the closed window's events are discarded. |

## AmigaOS calls

| Effect of the bug in the original | With the port |
|---|---|
| [A program hung at its start if an AmigaOS library it calls was missing (#44)](https://github.com/fubar-coder/bb2-c/issues/44). | It starts; only a call of the missing library then crashes. |
| [A program that calls a resource, such as the battery clock or the CIA resources, hung at its start (#45)](https://github.com/fubar-coder/bb2-c/issues/45). | The resource is opened. |
| [`CachePreDMA_` and `CachePostDMA_` passed their flags in the wrong register (#959)](https://github.com/fubar-coder/bb2-c/issues/959). | The flags reach the system. |

## RWE userlibs

These fixes concern the
[RWE userlibs]({{< relref "/docs/libraries/userlibs" >}}).

| Effect of the bug in the original | With the port |
|---|---|
| [`RIAnimInit`, `RIAnimDepth`, `RIAnimWidth` and `RIAnimHeight` crashed on some data that is not a `FORM ANIM` (#513)](https://github.com/fubar-coder/bb2-c/issues/513), such as an IFF `LIST` of type `ANIM`: the chunk search returned with an unbalanced stack. | The commands return normally; for such a `LIST`, which holds no animation they can read, `RIAnimDepth` returns 0. |
