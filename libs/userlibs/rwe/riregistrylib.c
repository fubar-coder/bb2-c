/*
 * riregistrylib - RWE userlib 3 (Red When Excited, 1997): the interface
 * to registry.library (REDRegistry), 35 commands.  The library itself
 * was only given out as a pre-release and exists nowhere; every command
 * is one call of it.
 *
 * C translation of rwelibsrc/Registry/Registry3.bb (a BB22 source,
 * converted by tools/bb22); reference: the reference build of the source
 * (REFERENCE refbuild, ref/riregistrylib.obj; no binary exists).  Notes:
 * docs/rwelibs/port/riregistrylib.md.
 *
 * init opens registry.library (any version; without it IsRegistryOpen
 * is 0 and everything returns 0), finit unlocks every registry structure
 * still locked and closes it.  LockRegistry pushes the structure it gets
 * on a stack of 16 longs (curstruct: byte offset, the first one at 4);
 * every other call goes through CallLibrary with a0 = the current
 * structure (regstruct; 0: nothing is called, the result is 0) and a6 =
 * the library base, the arguments in the registers the source loads:
 *   -30 LockRegistry d0              -36 UnlockRegistry
 *   -42 SetRegistryBaseLibrary a1    -48 ExistsLibrary a1,d0=0
 *   -54 GetLibraryValue a1,d0=0      -60 GetRegistryVariable a1,d0=0
 *   -66 SetRegistryVariable a1,a2,d0=type,d1=size
 *   -72 ChangeRegistryVariable a1,a2,d0,d1
 *   -78 NewRegistryLibrary a1,a2,a3  -84 ParseRegistryLibrary a1
 *   -96 NextParseItem                -102 ChangeRegistryAccess
 *   -108 DeleteRegistryLibrary a1    -114 DeleteRegistryVariable a1
 *   -120 NumRegistryLibraryVariables a1
 *   -126 SaveRegistryLibrary a1,a2   -132 LoadRegistryLibrary a1,a2
 *   -138 DeleteRegistryLibraryVariables a1
 *   -144 SetLibraryValue a1,a2       -150 SetRegistryVariableIndex
 *                                         a1,a2,d0,d1,d2
 *   -156 RenameRegistryVariable a1,a2
 *   -162 SaveLibraryVariables a1,a2  -174 RenameRegistryLibrary a1,a2
 *   -180 InsertNewRegistryLibrary a1,a2,a3,a4
 *   -186 GetStructureVariables a1,a2 -192 FreeStructureVariables a1,a2
 *   -198 LoadLibraryVariables a1,a2
 * The string results come from the structure: +14 the name (ParseItemName$),
 * +18.w the type (1 string, 2 data, 3 boolean, 4 integer), +24 the value
 * (a string for type 1, the number for types 3/4).
 *
 * C behind thunks; the library calls through regjsr() (inline assembler:
 * a3/a4 are the fixed registers of the Blitz program, loaded for the call
 * and restored after it).  The registers the source does not load are
 * not handed through (the original passes the caller's on; a library
 * that keeps to the interface above does not see a difference).
 *
 * BUG(orig), kept:
 *   - The structure stack has no limits: LockRegistry beyond 15 levels
 *     writes over curstruct and regstruct behind it; UnlockRegistry
 *     without a lock goes below it - into the end of the token table
 *     (the libfin's Dc.l 0, then $0000FFFF: unlocked again, that is
 *     "unlocked" with a0 = $FFFF and written over with 0) (#781).
 *   - GetLibraryValue$/GetRegistryStringVariable without a structure read
 *     the type and value at 18/24 of address 0 (CallLibrary's 0 does not
 *     stop them) (#780).
 *   - The string results are copied with their 0 and leave a3 behind it
 *     (#783).
 *   - SetRegistryVariable name,value$ and SetRegistryInt/BoolVariable
 *     pass the caller's d2 as the size (#782).
 *   - finit leaves regstruct (and the base) set (#779).
 * LockRegistry stores the result also when it is 0 (the following calls
 * then do nothing, but UnlockRegistry still pops it): no defect, it keeps
 * LockRegistry and UnlockRegistry balanced.
 * DIFF:
 *   - NewRegistryLibrary and InsertNewRegistryLibrary restore a3/a4
 *     after the call (the original leaves the file name in a3 and the
 *     "insert before" name in a4 - the program's string pointer and
 *     locals).
 *   - Below the stack, the C build has the two longs of the original
 *     ($0000FFFF, 0); further below (a third UnlockRegistry without a
 *     lock and more) it reads other memory than the original's table.
 *   - a2 stays unchanged by the string functions (the original takes
 *     the argument's length from it), as in all C ports.
 */
#include "blitzlib.h"
#include "blitzexec.h"

/* entry points (thunks, defined below) */
void _isregistryopen(void), _lockregistry(void), _unlockregistry(void);
void _setregistrybaselibrary(void), _existsregistrylibrary(void), _getlibraryvalue(void);
void _getregistryvariable(void), _setregistryvariable(void), _setregistryvariable2(void);
void _changeregistryvariable(void), _newregistrylibrary(void), _newregistrylibrary2(void);
void _parseregistrylibrary(void), _nextparseitem(void), _changeregistryaccess(void);
void _deleteregistrylibrary(void), _deleteregistryvariable(void);
void _numregistrylibraryvariables(void), _saveregistrylibrary(void);
void _loadregistrylibrary(void), _getregistrystringvariable(void);
void _setregistryboolvariable(void), _parseitemname(void), _parseitemvalue(void);
void _getregistryboolvariable(void), _setregistryintvariable(void);
void _getregistryintvariable(void), _deleteregistrylibraryvariables(void);
void _setlibraryvalue(void), _setregistryvariableindex(void), _renameregistryvariable(void);
void _savelibraryvariables(void), _renameregistrylibrary(void);
void _insertnewregistrylibrary(void), _insertnewregistrylibrary2(void);
void _getstructurevariables(void), _freestructurevariables(void);
void _loadlibraryvariables(void), _libinit(void), _libfinit(void);

BLITZ_ENTRY_DECL(init);
BLITZ_ENTRY_DECL(finit);

#define S BL_STRING
#define LG BL_LONG

/* !libheader {#riregistrylib,init,0,finit,0} */
BLITZ_LIBHEADER(LIB_riregistrylib, init, BL_NORET, finit, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_isregistryopen, 0, 0);
BLITZ_NAME("IsRegistryOpen", "-- returns 0 if registry not opened");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(LG);
BLITZ_LIBS0();
BLITZ_SUBS(_lockregistry, 0, 0);
BLITZ_NAME("LockRegistry", "AccessMode");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_unlockregistry, 0, 0);
BLITZ_NAME("UnlockRegistry", "-- unlock and release the registry");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_setregistrybaselibrary, 0, 0);
BLITZ_NAME("SetRegistryBaseLibrary", "LibraryName");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_existsregistrylibrary, 0, 0);
BLITZ_NAME("ExistsRegistryLibrary", "LibraryName");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_getlibraryvalue, 0, 0);
BLITZ_NAME("GetLibraryValue$", "LibraryName");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_getregistryvariable, 0, 0);
BLITZ_NAME("GetRegistryVariable", "VariableName");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_setregistryvariable, 0, 0);
BLITZ_ARGS(S, LG, LG);
BLITZ_LIBS0();
BLITZ_SUBS(_setregistryvariable2, 0, 0);
BLITZ_NAME("SetRegistryVariable", "VariableName,Value$ OR VariableName,Address,ByteSize");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, LG, LG, LG);
BLITZ_LIBS0();
BLITZ_SUBS(_changeregistryvariable, 0, 0);
BLITZ_NAME("ChangeRegistryVariable", "VariableName,Data,Type,ByteSize");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_newregistrylibrary, 0, 0);
BLITZ_ARGS(S, S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_newregistrylibrary2, 0, 0);
BLITZ_NAME("NewRegistryLibrary", "LibraryName,LibraryValue[,VariableFilename]");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_parseregistrylibrary, 0, 0);
BLITZ_NAME("ParseRegistryLibrary", "LibraryName");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_nextparseitem, 0, 0);
BLITZ_NAME("NextParseItem", "");

BLITZ_ASTATEMENT();
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_changeregistryaccess, 0, 0);
BLITZ_NAME("ChangeRegistryAccess", "-- Change registry to read access");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_deleteregistrylibrary, 0, 0);
BLITZ_NAME("DeleteRegistryLibrary", "LibraryName");

BLITZ_ACOMMAND(BL_WORD);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_deleteregistryvariable, 0, 0);
BLITZ_NAME("DeleteRegistryVariable", "VariableName");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_numregistrylibraryvariables, 0, 0);
BLITZ_NAME("NumRegistryLibraryVariables", "VariableName");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_saveregistrylibrary, 0, 0);
BLITZ_NAME("SaveRegistryLibrary", "LibraryName,FileName");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_loadregistrylibrary, 0, 0);
BLITZ_NAME("LoadRegistryLibrary", "LibraryName,FileName");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_getregistrystringvariable, 0, 0);
BLITZ_NAME("GetRegistryStringVariable", "VariableName");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, LG);
BLITZ_LIBS0();
BLITZ_SUBS(_setregistryboolvariable, 0, 0);
BLITZ_NAME("SetRegistryBoolVariable", "VariableName,True/False");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_parseitemname, 0, 0);
BLITZ_NAME("ParseItemName$", "-- return the name associated with the last parse");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(_parseitemvalue, 0, 0);
BLITZ_NAME("ParseItemValue$", "-- return the string value associated with the last parse");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_getregistryboolvariable, 0, 0);
BLITZ_NAME("GetRegistryBoolVariable", "VariableName");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, LG);
BLITZ_LIBS0();
BLITZ_SUBS(_setregistryintvariable, 0, 0);
BLITZ_NAME("SetRegistryIntVariable", "VariableName,#");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_getregistryintvariable, 0, 0);
BLITZ_NAME("GetRegistryIntVariable", "VariableName");

BLITZ_ASTATEMENT();
BLITZ_ARGS(S);
BLITZ_LIBS0();
BLITZ_SUBS(_deleteregistrylibraryvariables, 0, 0);
BLITZ_NAME("DeleteRegistryLibraryVariables", "LibraryName");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_setlibraryvalue, 0, 0);
BLITZ_NAME("SetLibraryValue", "LibraryName,Value");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, LG, LG, LG, LG);
BLITZ_LIBS0();
BLITZ_SUBS(_setregistryvariableindex, 0, 0);
BLITZ_NAME("SetRegistryVariableIndex", "VariableName,Data,Type,ByteSize,Index");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_renameregistryvariable, 0, 0);
BLITZ_NAME("RenameRegistryVariable", "VariableName,NewVariableName");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_savelibraryvariables, 0, 0);
BLITZ_NAME("SaveLibraryVariables", "LibraryName,Filename");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_renameregistrylibrary, 0, 0);
BLITZ_NAME("RenameRegistryLibrary", "LibraryName,NewLibraryName");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_insertnewregistrylibrary, 0, 0);
BLITZ_ARGS(S, S, S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_insertnewregistrylibrary2, 0, 0);
BLITZ_NAME("InsertNewRegistryLibrary",
           "LibraryName,LibraryValue,InsertBefore[,VariableFilename]");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(LG, LG);
BLITZ_LIBS0();
BLITZ_SUBS(_getstructurevariables, 0, 0);
BLITZ_NAME("GetStructureVariables", "Description,Structure");

BLITZ_ASTATEMENT();
BLITZ_ARGS(LG, LG);
BLITZ_LIBS0();
BLITZ_SUBS(_freestructurevariables, 0, 0);
BLITZ_NAME("FreeStructureVariables", "-- release data allocated by GetStructureVariables");

BLITZ_ACOMMAND(BL_LONG);
BLITZ_ARGS(S, S);
BLITZ_LIBS0();
BLITZ_SUBS(_loadlibraryvariables, 0, 0);
BLITZ_NAME("LoadLibraryVariables", "LibraryName,Filename");

BLITZ_NULLSUB_AT(finit, _libfinit, 0, 0);
BLITZ_NULLSUB_AT(init, _libinit, 0, 0);

BLITZ_LIBFIN();

/* ---------------------------------------------------------------- data */

/* the LVOs of registry.library the source uses */
#define LVO_LockRegistry                   (-30)
#define LVO_UnlockRegistry                 (-36)
#define LVO_SetRegistryBaseLibrary         (-42)
#define LVO_ExistsLibrary                  (-48)
#define LVO_GetLibraryValue                (-54)
#define LVO_GetRegistryVariable            (-60)
#define LVO_SetRegistryVariable            (-66)
#define LVO_ChangeRegistryVariable         (-72)
#define LVO_NewRegistryLibrary             (-78)
#define LVO_ParseRegistryLibrary           (-84)
#define LVO_NextParseItem                  (-96)
#define LVO_ChangeRegistryAccess           (-102)
#define LVO_DeleteRegistryLibrary          (-108)
#define LVO_DeleteRegistryVariable         (-114)
#define LVO_NumRegistryLibraryVariables    (-120)
#define LVO_SaveRegistryLibrary            (-126)
#define LVO_LoadRegistryLibrary            (-132)
#define LVO_DeleteRegistryLibraryVariables (-138)
#define LVO_SetLibraryValue                (-144)
#define LVO_SetRegistryVariableIndex       (-150)
#define LVO_RenameRegistryVariable         (-156)
#define LVO_SaveLibraryVariables           (-162)
#define LVO_RenameRegistryLibrary          (-174)
#define LVO_InsertNewRegistryLibrary       (-180)
#define LVO_GetStructureVariables          (-186)
#define LVO_FreeStructureVariables         (-192)
#define LVO_LoadLibraryVariables           (-198)

/* RegistryStructure */
#define RS_NAME 14
#define RS_TYPE 18
#define RS_DATA 24
#define VAR_STRING 1
#define VAR_DATA 2
#define VAR_BOOLEAN 3
#define VAR_INTEGER 4

/* structs, curstruct, regstruct, regbase in the original's order (a
   stack overflow runs into curstruct and regstruct), with the two longs
   in front of structs that the original has there (the end of its token
   table: !libfin's $FFFF and Dc.l 0, behind the long 0 of the last
   entry), for UnlockRegistry without a lock */
static struct BL_PACKED {
    ULONG table[2];
    ULONG structs[16];
    UWORD curstruct;
    ULONG regstruct;
    struct Library *regbase;
} st = { .table = { 0x0000ffff, 0 } };

/* structs(pc) + curstruct (a word index) */
#define SLOT() (*(ULONG *)((UBYTE *)st.structs + (WORD)st.curstruct))

#define LONGAT(p, o) (*(ULONG *)((UBYTE *)(p) + (o)))
#define WORDAT(p, o) (*(UWORD *)((UBYTE *)(p) + (o)))

/* regjsr(off, a0, d0, d1, d2, a1, a2, a3, a4): JSR off(a6) with a6 =
   regbase and the registers given.  a3/a4 are the fixed registers of the
   Blitz program (-ffixed-a3/a4): loaded from memory for the call and
   restored after it.  The library may change d0/d1/a0/a1. */
static ULONG call_a3a4[2];

static ULONG regjsr(WORD off, ULONG a0, ULONG d0, ULONG d1, ULONG d2, ULONG a1, ULONG a2,
                    ULONG a3, ULONG a4)
{
#ifndef BL_CODE_MODEL
    register ULONG _d0 __asm("d0") = d0;
    register ULONG _d1 __asm("d1") = d1;
    register ULONG _d2 __asm("d2") = d2;
    register LONG _d7 __asm("d7") = off;
    register ULONG _a0 __asm("a0") = a0;
    register ULONG _a1 __asm("a1") = a1;
    register ULONG _a2 __asm("a2") = a2;
    register struct Library *_a6 __asm("a6") = st.regbase;

    call_a3a4[0] = a3;
    call_a3a4[1] = a4;
    __asm volatile("movem.l a3-a4,-(sp)\n\t"
                   "movem.l %8,a3-a4\n\t"
                   "jsr     0(a6,d7.w)\n\t"
                   "movem.l (sp)+,a3-a4"
        : "+d"(_d0), "+d"(_d1), "+a"(_a0), "+a"(_a1)
        : "d"(_d2), "d"(_d7), "a"(_a2), "a"(_a6), "m"(call_a3a4)
        : "cc", "memory");
    return _d0;
#else
    (void)off; (void)a0; (void)d1; (void)d2; (void)a1; (void)a2; (void)a3; (void)a4;
    return d0;
#endif
}

/* CallLibrary: a0 = regstruct; nothing (0) without a structure.  The
   registers the source does not load are the caller's a3/a4 (passed
   through) and 0 */
static ULONG calllib(WORD off, ULONG d0, ULONG d1, ULONG d2, ULONG a1, ULONG a2, ULONG a3,
                     ULONG a4)
{
    if (!st.regstruct)
        return 0;
    return regjsr(off, st.regstruct, d0, d1, d2, a1, a2, a3, a4);
}
#define CALL(off, d0, d1, d2, a1, a2) \
    calllib(off, d0, d1, d2, a1, a2, (ULONG)blitz_string, (ULONG)blitz_locals)

/* retstring: the string s (with its 0) to a3, which stays behind it;
   d0 = its length (0 for no string) */
static ULONG retstring(const UBYTE *s)
{
    ULONG n;

    if (!s)
        return 0;
    n = (ULONG)-1;
    do
        n++;
    while ((*blitz_string++ = *s++) != 0);
    return n;
}

/* returnstring: the value of the structure if it is a string -
   BUG(orig): without a structure 18/24 of address 0 (#780) */
static ULONG returnstring(void)
{
    UBYTE *rs = (UBYTE *)st.regstruct;
    const UBYTE *v = (const UBYTE *)LONGAT(rs, RS_DATA);

    if (!v || WORDAT(rs, RS_TYPE) != VAR_STRING)
        return 0;
    return retstring(v);
}

/* CORRECTa3: the result goes where the string argument is */
#define CORRECTA3(lens) (blitz_string -= (lens)[-1] + 1)

/* ------------------------------------------------------- init / finit */

static void BLITZ_ROUTINE libinit_c(void)
{
    st.regbase = OpenLibrary((CONST_STRPTR)"registry.library", 0);
}

/* unlock structs[curstruct] ... structs[4] */
static void BLITZ_ROUTINE libfinit_c(void)
{
    WORD i;

    if (!st.regbase)
        return;
    i = (WORD)st.curstruct;
    if (i > 0) {
        do {
            ULONG *p = (ULONG *)((UBYTE *)st.structs + i);
            if (*p) {
                regjsr(LVO_UnlockRegistry, *p, (UWORD)i, 0, 0, (ULONG)st.structs, 0,
                       (ULONG)blitz_string, (ULONG)blitz_locals);
                *p = 0;
            }
            i -= 4;
        } while (i != 0);
        st.curstruct = 0;
    }
    CloseLibrary(st.regbase);
}

/* ------------------------------------------------------------ commands */

static ULONG BLITZ_ROUTINE isregistryopen_c(void)
{
    return (ULONG)st.regbase;
}

/* LockRegistry: the result pushed (also 0) */
static ULONG BLITZ_ROUTINE lockregistry_c(ULONG mode BL_REG(d0))
{
    ULONG r;

    if (!st.regbase)
        return 0;
    r = regjsr(LVO_LockRegistry, 0, mode, 0, 0, 0, 0, (ULONG)blitz_string,
               (ULONG)blitz_locals);
    st.curstruct += 4;
    st.regstruct = r;
    SLOT() = st.regstruct;
    return r;
}

/* UnlockRegistry: pop (BUG(orig): also below the stack, #781) */
static void BLITZ_ROUTINE unlockregistry_c(void)
{
    CALL(LVO_UnlockRegistry, 0, 0, 0, 0, 0);
    st.regstruct = 0;
    SLOT() = st.regstruct;
    st.curstruct -= 4;
    st.regstruct = SLOT();
}

static ULONG BLITZ_ROUTINE setregistrybaselibrary_c(ULONG name BL_REG(d0))
{
    return CALL(LVO_SetRegistryBaseLibrary, name, 0, 0, name, 0);
}

static ULONG BLITZ_ROUTINE existsregistrylibrary_c(ULONG name BL_REG(d0))
{
    return CALL(LVO_ExistsLibrary, 0, 0, 0, name, 0);
}

/* GetLibraryValue$ / GetRegistryStringVariable: the call's result does
   not count, the structure's string value does */
static ULONG getstring(WORD off, ULONG name, const ULONG *lens)
{
    CORRECTA3(lens);
    CALL(off, 0, 0, 0, name, 0);
    return returnstring();
}

static ULONG BLITZ_ROUTINE getlibraryvalue_c(ULONG name BL_REG(d0),
                                             const ULONG *lens BL_REG(a2))
{
    return getstring(LVO_GetLibraryValue, name, lens);
}

static ULONG BLITZ_ROUTINE getregistrystringvariable_c(ULONG name BL_REG(d0),
                                                       const ULONG *lens BL_REG(a2))
{
    return getstring(LVO_GetRegistryVariable, name, lens);
}

static ULONG BLITZ_ROUTINE getregistryvariable_c(ULONG name BL_REG(d0))
{
    return CALL(LVO_GetRegistryVariable, 0, 0, 0, name, 0);
}

/* doset: a1 = name, a2 = value, d0 = type, d1 = size (the caller's d2) */
static ULONG doset(ULONG type, ULONG name, ULONG value, ULONG size)
{
    return CALL(LVO_SetRegistryVariable, type, size, 0, name, value);
}

static ULONG BLITZ_ROUTINE setregistryintvariable_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1),
                                                    ULONG d2 BL_REG(d2))
{
    return doset(VAR_INTEGER, name, v, d2);
}

static ULONG BLITZ_ROUTINE setregistryboolvariable_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1),
                                                     ULONG d2 BL_REG(d2))
{
    return doset(VAR_BOOLEAN, name, v, d2);
}

static ULONG BLITZ_ROUTINE setregistryvariable_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1),
                                                 ULONG d2 BL_REG(d2))
{
    return doset(VAR_STRING, name, v, d2);
}

static ULONG BLITZ_ROUTINE setregistryvariable2_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1),
                                                  ULONG size BL_REG(d2))
{
    return doset(VAR_DATA, name, v, size);
}

static ULONG BLITZ_ROUTINE changeregistryvariable_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1),
                                                    ULONG type BL_REG(d2), ULONG size BL_REG(d3))
{
    return CALL(LVO_ChangeRegistryVariable, type, size, 0, name, v);
}

/* NewRegistryLibrary: a3 = the variable file name (form 1: 0) */
static ULONG BLITZ_ROUTINE newregistrylibrary2_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1),
                                                 ULONG file BL_REG(d2))
{
    return calllib(LVO_NewRegistryLibrary, name, v, file, name, v, file, (ULONG)blitz_locals);
}

static ULONG BLITZ_ROUTINE newregistrylibrary_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1))
{
    return newregistrylibrary2_c(name, v, 0);
}

/* the calls with a1 = d0 (the name) */
#define NAMECALL(f, off)                                                    \
    static ULONG BLITZ_ROUTINE f(ULONG name BL_REG(d0))                     \
    {                                                                       \
        return CALL(off, name, 0, 0, name, 0);                              \
    }
NAMECALL(parseregistrylibrary_c, LVO_ParseRegistryLibrary)
NAMECALL(deleteregistrylibrary_c, LVO_DeleteRegistryLibrary)
NAMECALL(deleteregistryvariable_c, LVO_DeleteRegistryVariable)
NAMECALL(numregistrylibraryvariables_c, LVO_NumRegistryLibraryVariables)
NAMECALL(deleteregistrylibraryvariables_c, LVO_DeleteRegistryLibraryVariables)

/* the calls with a1 = d0, a2 = d1 */
#define NAME2CALL(f, off)                                                   \
    static ULONG BLITZ_ROUTINE f(ULONG a BL_REG(d0), ULONG b BL_REG(d1))    \
    {                                                                       \
        return CALL(off, a, b, 0, a, b);                                    \
    }
NAME2CALL(saveregistrylibrary_c, LVO_SaveRegistryLibrary)
NAME2CALL(loadregistrylibrary_c, LVO_LoadRegistryLibrary)
NAME2CALL(setlibraryvalue_c, LVO_SetLibraryValue)
NAME2CALL(renameregistryvariable_c, LVO_RenameRegistryVariable)
NAME2CALL(savelibraryvariables_c, LVO_SaveLibraryVariables)
NAME2CALL(renameregistrylibrary_c, LVO_RenameRegistryLibrary)
NAME2CALL(getstructurevariables_c, LVO_GetStructureVariables)
NAME2CALL(freestructurevariables_c, LVO_FreeStructureVariables)
NAME2CALL(loadlibraryvariables_c, LVO_LoadLibraryVariables)

static ULONG BLITZ_ROUTINE nextparseitem_c(void)
{
    return CALL(LVO_NextParseItem, 0, 0, 0, 0, 0);
}

static ULONG BLITZ_ROUTINE changeregistryaccess_c(void)
{
    return CALL(LVO_ChangeRegistryAccess, 0, 0, 0, 0, 0);
}

/* GetRegistryInt/BoolVariable: the value of a boolean or integer */
static ULONG BLITZ_ROUTINE getregistryintvariable_c(ULONG name BL_REG(d0))
{
    UWORD type = (UWORD)CALL(LVO_GetRegistryVariable, 0, 0, 0, name, 0);

    if (type != VAR_BOOLEAN && type != VAR_INTEGER)
        return 0;
    return LONGAT(st.regstruct, RS_DATA);
}

static ULONG BLITZ_ROUTINE setregistryvariableindex_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1),
                                                      ULONG type BL_REG(d2),
                                                      ULONG size BL_REG(d3),
                                                      ULONG index BL_REG(d4))
{
    return CALL(LVO_SetRegistryVariableIndex, type, size, index, name, v);
}

/* InsertNewRegistryLibrary: a3 = the variable file name (form 1: 0), a4
   = insert before */
static ULONG BLITZ_ROUTINE insertnewregistrylibrary2_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1),
                                                       ULONG before BL_REG(d2),
                                                       ULONG file BL_REG(d3))
{
    return calllib(LVO_InsertNewRegistryLibrary, name, v, before, name, v, file, before);
}

static ULONG BLITZ_ROUTINE insertnewregistrylibrary_c(ULONG name BL_REG(d0), ULONG v BL_REG(d1),
                                                      ULONG before BL_REG(d2))
{
    return insertnewregistrylibrary2_c(name, v, before, 0);
}

/* ParseItemName$: the structure's name */
static ULONG BLITZ_ROUTINE parseitemname_c(void)
{
    if (!st.regstruct)
        return 0;
    return retstring((const UBYTE *)LONGAT(st.regstruct, RS_NAME));
}

/* ParseItemValue$: the structure's string value */
static ULONG BLITZ_ROUTINE parseitemvalue_c(void)
{
    if (!st.regstruct)
        return 0;
    return returnstring();
}

/* Registers: the original changes d7 (the LVO), a6 (the base), a0/a1 (and
   what the library changes: d0/d1/a0/a1), a2-a4 where it loads them, d2
   (NewRegistryLibrary form 1), d3 (InsertNewRegistryLibrary form 1), a3
   (the string functions).  The thunks keep d0 (statements), d1/a0/a1,
   the C code d2-d7/a2-a6 (a3 but in the string functions). */
BLITZ_THUNK(_libinit, libinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_libfinit, libfinit_c, "d0-d1/a0-a1");
BLITZ_THUNK(_isregistryopen, isregistryopen_c, "d1/a0-a1");
BLITZ_THUNK(_lockregistry, lockregistry_c, "d1/a0-a1");
BLITZ_THUNK(_unlockregistry, unlockregistry_c, "d0-d1/a0-a1");
BLITZ_THUNK(_setregistrybaselibrary, setregistrybaselibrary_c, "d1/a0-a1");
BLITZ_THUNK(_existsregistrylibrary, existsregistrylibrary_c, "d1/a0-a1");
BLITZ_THUNK(_getlibraryvalue, getlibraryvalue_c, "d1/a0-a1");
BLITZ_THUNK(_getregistryvariable, getregistryvariable_c, "d1/a0-a1");
BLITZ_THUNK(_setregistryvariable, setregistryvariable_c, "d1/a0-a1");
BLITZ_THUNK(_setregistryvariable2, setregistryvariable2_c, "d1/a0-a1");
BLITZ_THUNK(_changeregistryvariable, changeregistryvariable_c, "d1/a0-a1");
BLITZ_THUNK(_newregistrylibrary, newregistrylibrary_c, "d1/a0-a1");
BLITZ_THUNK(_newregistrylibrary2, newregistrylibrary2_c, "d1/a0-a1");
BLITZ_THUNK(_parseregistrylibrary, parseregistrylibrary_c, "d1/a0-a1");
BLITZ_THUNK(_nextparseitem, nextparseitem_c, "d1/a0-a1");
BLITZ_THUNK(_changeregistryaccess, changeregistryaccess_c, "d0-d1/a0-a1");
BLITZ_THUNK(_deleteregistrylibrary, deleteregistrylibrary_c, "d1/a0-a1");
BLITZ_THUNK(_deleteregistryvariable, deleteregistryvariable_c, "d1/a0-a1");
BLITZ_THUNK(_numregistrylibraryvariables, numregistrylibraryvariables_c, "d1/a0-a1");
BLITZ_THUNK(_saveregistrylibrary, saveregistrylibrary_c, "d1/a0-a1");
BLITZ_THUNK(_loadregistrylibrary, loadregistrylibrary_c, "d1/a0-a1");
BLITZ_THUNK(_getregistrystringvariable, getregistrystringvariable_c, "d1/a0-a1");
BLITZ_THUNK(_setregistryboolvariable, setregistryboolvariable_c, "d1/a0-a1");
BLITZ_THUNK(_parseitemname, parseitemname_c, "d1/a0-a1");
BLITZ_THUNK(_parseitemvalue, parseitemvalue_c, "d1/a0-a1");
BLITZ_THUNK(_getregistryboolvariable, getregistryintvariable_c, "d1/a0-a1");
BLITZ_THUNK(_setregistryintvariable, setregistryintvariable_c, "d1/a0-a1");
BLITZ_THUNK(_getregistryintvariable, getregistryintvariable_c, "d1/a0-a1");
BLITZ_THUNK(_deleteregistrylibraryvariables, deleteregistrylibraryvariables_c, "d0-d1/a0-a1");
BLITZ_THUNK(_setlibraryvalue, setlibraryvalue_c, "d1/a0-a1");
BLITZ_THUNK(_setregistryvariableindex, setregistryvariableindex_c, "d1/a0-a1");
BLITZ_THUNK(_renameregistryvariable, renameregistryvariable_c, "d1/a0-a1");
BLITZ_THUNK(_savelibraryvariables, savelibraryvariables_c, "d1/a0-a1");
BLITZ_THUNK(_renameregistrylibrary, renameregistrylibrary_c, "d1/a0-a1");
BLITZ_THUNK(_insertnewregistrylibrary, insertnewregistrylibrary_c, "d1/a0-a1");
BLITZ_THUNK(_insertnewregistrylibrary2, insertnewregistrylibrary2_c, "d1/a0-a1");
BLITZ_THUNK(_getstructurevariables, getstructurevariables_c, "d1/a0-a1");
BLITZ_THUNK(_freestructurevariables, freestructurevariables_c, "d0-d1/a0-a1");
BLITZ_THUNK(_loadlibraryvariables, loadlibraryvariables_c, "d1/a0-a1");
