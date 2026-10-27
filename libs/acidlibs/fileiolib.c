/*
 * fileiolib - command library 124: files - OpenFile, ReadFile, WriteFile,
 * CloseFile, KillFile, Fields, Put, Get, FileOutput, FileInput, Lof,
 * FileSeek, Loc, Eof, DosBuffLen, WriteMem, ReadMem, Exists.
 *
 * C translation of bblibsrc/fileiolib (March 1995, BUM 6; binary checked:
 * same as the source).
 *   init: a nullsub in the middle of the table (`init:`), d0 = dos base
 *         (!libs #doslib,#ld0), kept for the I/O handlers.
 * An object library (!libfin{_toke,_load,_save,_use,_free,5,5}): the
 * Blitz object "File" (dumtoke), default maximum 5, objects of 2^5 = 32
 * bytes, zeroed at program start; _free = CloseFile, called for every
 * object at program end.  The compiler loads the object pointer from the
 * file number: $1180 = a1, $1280 = a2, $1380 = a3 (parameter 0).
 *
 * A file (struct bfile):
 *   +0  dos handle (0 = not open)    +16 own dos buffer
 *   +4  record length (Fields)       +20.w its length (DosBuffLen)
 *   +8  unused                       +22.w buffer flags: < 0 written to
 *   +12 list of record fields        +24.w valid bytes in the buffer
 *       {next, address, length}      +26.w position in the buffer
 *                                    +28 file position of the buffer
 * All reading and writing goes through the buffer ("the following
 * routines allow for the simple fact that DOS is FUCT"): myread, mywrite,
 * myseek, myflush - word arithmetic as in the original.
 *
 * FileInput/FileOutput hand inputoutputlib a handler (ALibJsr
 * #inputoutputlib LSL 7+3/+4, a0 = handler) with BRA entries in front:
 *   input:  -4 string (a0 = buffer, d0 = max -> d0 = length, up to CR/LF),
 *            0 one character (-> d0, -1 at the end)
 *   output: -8 0-terminated string (a0), -4 string (a0, d0 = length),
 *            0 one character (d0)
 * They keep the registers of the original (thunks); the file is the one
 * of the last FileInput/FileOutput (pointer to the object).
 *
 * Kept from the original (BUG(orig)):
 *   - OpenFile/ReadFile/WriteFile first close the File#; if that file had
 *     Fields, the original loop leaves a2 = 0, and the name length is read
 *     from -4(0) (on the A1200's 24 bit bus the last long of the ROM),
 *     then name[(WORD)length] is cleared.  Nachgebildet.
 *   - The dos buffer (memlib globalloc, MEMF_PUBLIC) is not checked (in
 *     the original globalloc never returns 0: memlib err1 ends the
 *     program; only the C memlib returns 0, K).
 *   - CloseFile frees the record fields but leaves the pointer to them
 *     and the record length; OpenFile clears them.  Fields adds to the
 *     existing fields and record length (no reset); the list is built
 *     backwards, Put/Get transfer the fields in reverse order of Fields.
 *   - Fields: types 1/2/7 have 1/2/(long at address-8)+6 bytes, all
 *     others 4; strings start at address-4.
 *   - Exists: 0 for a missing file AND for an empty file, -1 for a drawer
 *     (fib_DirEntryType = 2; a volume's root (1) gives its fib_Size); the
 *     FileInfoBlock comes from memlib globalloc (not checked, see above).
 *   - myseek/myread/mywrite return d0 unchanged when there is nothing to
 *     do (length 0, position in the buffer): FileSeek/Put/Get compare the
 *     file number resp. the field address with -1 then.
 *   - Lof does not flush: the larger of the dos length and the end of the
 *     buffer.
 *   - Eof at the end of the file: myseek has already set the buffer's
 *     file position one further when the dos Seek fails, and nothing
 *     resets it - Loc and Lof are 1 too high afterwards (t50: "written
 *     lof 32 loc 32" for a file of 31 bytes; Blitz evaluates the NPrint
 *     arguments from the right, Eof first).
 * DIFF:
 *   - The original jumps from the normal code into the error check area
 *     (err1/err2 in the I/O handlers, err3 Put/Get, err6 FileSeek) and
 *     uses the message of err7 from there: without the debugger that area
 *     is cut away and the jumps go behind the library.  Here these runtime
 *     errors and their messages are in the normal code (with the debugger:
 *     same message, same TRAP #0).
 *   - a2 stays unchanged (Fields, OpenFile read it with a cursor), d4-d7/a3
 *     are not changed (the original changes d4 in Exists, d5-d7 in
 *     OpenFile/Fields/Put/Get, a2 in CloseFile).
 *   - The unused check reclenchk is not ported.
 */
#include "blitzlib.h"
#include <dos/dos.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dosbase
#include <proto/dos.h>
#undef __NOLIBBASE__

/* a record field */
struct rec {
    struct rec *next;                           /* 00.l */
    UBYTE *data;                                /* 04.l */
    ULONG len;                                  /* 08.l */
};

/* a file object, 2^5 bytes */
struct bfile {
    BPTR fh;                                    /* 00.l Dos Handle */
    ULONG reclen;                               /* 04.l length of record */
    ULONG unused;                               /* 08.l */
    struct rec *recs;                           /* 12.l record fields */
    UBYTE *buf;                                 /* 16.l mydos buffer */
    UWORD buflen;                               /* 20.w len of mydos */
    WORD flags;                                 /* 22.w <0: written to */
    UWORD valid;                                /* 24.w valid chars */
    UWORD pos;                                  /* 26.w offset in buffer */
    ULONG bufstart;                             /* 28.l file pos of buffer */
} BL_PACKED;

/* 22.l: flags and valid as one long (MOVE.l/CLR.l 22(a3)) */
#define FLAGSVALID(f) (*(ULONG *)&(f)->flags)

/* entry points (thunks / C, defined below) */
void dofileoutput(void), dofileinput(void), doloc(void), doeof(void);
void dodbufflen(void), codeinit(void), racchk(void), fchk(void), dbchk(void);
ULONG BLITZ_ROUTINE doexists(const UBYTE *name BL_REG(d0),
                             struct Library *dosbase BL_REG(a6));
void BLITZ_ROUTINE dowritemem(ULONG d0 BL_REG(d0), struct bfile *f BL_REG(a2),
                              const UBYTE *mem BL_REG(d1), ULONG len BL_REG(d2),
                              struct Library *dosbase BL_REG(a6));
void BLITZ_ROUTINE doreadmem(ULONG d0 BL_REG(d0), struct bfile *f BL_REG(a2),
                             UBYTE *mem BL_REG(d1), ULONG len BL_REG(d2),
                             struct Library *dosbase BL_REG(a6));
ULONG BLITZ_ROUTINE dolof(struct bfile *f BL_REG(a2),
                          struct Library *dosbase BL_REG(a6));
void BLITZ_ROUTINE dofileseek(ULONG d0 BL_REG(d0), struct bfile *f BL_REG(a2),
                              ULONG where BL_REG(d1),
                              struct Library *dosbase BL_REG(a6));
void BLITZ_ROUTINE dofreefile(struct Library *dosbase BL_REG(a6));
ULONG BLITZ_ROUTINE doopenfile(struct bfile *f BL_REG(a1), UBYTE *name BL_REG(d1),
                               const ULONG *lens BL_REG(a2),
                               struct Library *dosbase BL_REG(a6));
ULONG BLITZ_ROUTINE doreadfile(struct bfile *f BL_REG(a1), UBYTE *name BL_REG(d1),
                               const ULONG *lens BL_REG(a2),
                               struct Library *dosbase BL_REG(a6));
ULONG BLITZ_ROUTINE dowritefile(struct bfile *f BL_REG(a1), UBYTE *name BL_REG(d1),
                                const ULONG *lens BL_REG(a2),
                                struct Library *dosbase BL_REG(a6));
void BLITZ_ROUTINE dorecord(UWORD n BL_REG(d7), const UBYTE *args BL_REG(a2));
void BLITZ_ROUTINE doput(ULONG d0 BL_REG(d0), UWORD record BL_REG(d1),
                         struct Library *dosbase BL_REG(a6));
void BLITZ_ROUTINE doget(ULONG d0 BL_REG(d0), UWORD record BL_REG(d1),
                         struct Library *dosbase BL_REG(a6));
void BLITZ_ROUTINE dokillfile(const UBYTE *name BL_REG(d0),
                              struct Library *dosbase BL_REG(a6));

BLITZ_ENTRY_DECL(init);

/* !libheader{#fileiolib,init,0,0,errors} */
BLITZ_LIBHEADER(LIB_fileiolib, init, BL_NORET, 0, BLITZ_ERROR_AREA);

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_fileiolib, 0x1180, LIB_doslib, BL_LA(6));
BLITZ_SUBS(doopenfile, 0, 0);
BLITZ_NAME("OpenFile", "(File#,Filename$)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_fileiolib, 0x1380, LIB_doslib, BL_LA(6));
BLITZ_SUBS(dofreefile, 0, 0);
BLITZ_NAME("CloseFile", "File#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS(LIB_doslib, BL_LA(6));
BLITZ_SUBS(dokillfile, 0, 0);
BLITZ_NAME("KillFile", "Filename$");

BLITZ_ASTATEMENT();
BLITZ_REPARGS(1, 1, BL_WORD, BL_VARPTR);
BLITZ_LIBS(LIB_fileiolib, 0x1380);
BLITZ_SUBS(dorecord, 0, 0);
BLITZ_NAME("Fields", "File#,Var1[,Var2...]");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_fileiolib, 0x1380, LIB_doslib, BL_LA(6));
BLITZ_SUBS(doput, racchk, 0);
BLITZ_NAME("Put", "File#,Record");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_WORD);
BLITZ_LIBS(LIB_fileiolib, 0x1380, LIB_doslib, BL_LA(6));
BLITZ_SUBS(doget, racchk, 0);
BLITZ_NAME("Get", "File#,Record");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_fileiolib, 0x1380);
BLITZ_SUBS(dofileoutput, 0, 0);
BLITZ_NAME("FileOutput", "File#");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_fileiolib, 0x1380);
BLITZ_SUBS(dofileinput, 0, 0);
BLITZ_NAME("FileInput", "File#");

BLITZ_NULLSUB(0, 0, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_fileiolib, 0x1280, LIB_doslib, BL_LA(6));
BLITZ_SUBS(dolof, fchk, 0);
BLITZ_NAME("Lof", "(File#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG);
BLITZ_LIBS(LIB_fileiolib, 0x1280, LIB_doslib, BL_LA(6));
BLITZ_SUBS(dofileseek, fchk, 0);
BLITZ_NAME("FileSeek", "File#,Position");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_fileiolib, 0x1280);
BLITZ_SUBS(doloc, fchk, 0);
BLITZ_NAME("Loc", "(File#)");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS(LIB_fileiolib, 0x1280, LIB_doslib, BL_LA(6));
BLITZ_SUBS(doeof, fchk, 0);
BLITZ_NAME("Eof", "(File#)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(dodbufflen, dbchk, 0);
BLITZ_NAME("DosBuffLen", "Bytes");

BLITZ_NULLSUB_AT(init, codeinit, 0, 0, LIB_doslib, BL_LD(0));

BLITZ_DUMTOKE(_toke, "File", "");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_fileiolib, 0x1180, LIB_doslib, BL_LA(6));
BLITZ_SUBS(doreadfile, 0, 0);
BLITZ_NAME("ReadFile", "(File#,Filename$)");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS(BL_WORD, BL_STRING);
BLITZ_LIBS(LIB_fileiolib, 0x1180, LIB_doslib, BL_LA(6));
BLITZ_SUBS(dowritefile, 0, 0);
BLITZ_NAME("WriteFile", "(File#,Filename$)");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_fileiolib, 0x1280, LIB_doslib, BL_LA(6));
BLITZ_SUBS(dowritemem, fchk, 0);
BLITZ_NAME("WriteMem", "File#,Address,Length");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD, BL_LONG, BL_LONG);
BLITZ_LIBS(LIB_fileiolib, 0x1280, LIB_doslib, BL_LA(6));
BLITZ_SUBS(doreadmem, fchk, 0);
BLITZ_NAME("ReadMem", "File#,Address,Length");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS(LIB_doslib, BL_LA(6));
BLITZ_SUBS(doexists, 0, 0);
BLITZ_NAME("Exists", "(Filename$)");

BLITZ_ENTRY_DECL(_load);
BLITZ_ENTRY_DECL(_save);
BLITZ_ENTRY_DECL(_use);
BLITZ_ENTRY_DECL(_free);
BLITZ_NULLSUB_AT(_load, 0, 0, 0);
BLITZ_NULLSUB_AT(_save, 0, 0, 0);
BLITZ_NULLSUB_AT(_use, 0, 0, 0);
BLITZ_NULLSUB_AT(_free, dofreefile, 0, 0, LIB_doslib, BL_LA(6));

/* !libfin{_toke,_load,_save,_use,_free,5,5} */
BLITZ_LIBFIN7(_toke.n, _load, _save, _use, _free, 5, 5);

/* ********************** CODE BELOW ************************ */

static struct Library *dosbase_g;               /* dosbase: Dc.l 0 */
static UBYTE temp;                              /* temp: Dc 0 */
static struct bfile *infile, *outfile;          /* Dc.l 0 */
static ULONG doslen = 2048;                     /* size of mydos buffer */

/* runtime errors reached from the normal code (DIFF, see above) */
static const char mess1[] = "Invalid Input File";
static const char mess2[] = "Invalid Output File";
static const char mess3[] = "Can't Find Record";
static const char mess6[] = "Can't FileSeek Position";
static const char mess7[] = "Error Reading/Writing File";

/* --------------------------------------------- the buffered dos layer */

/* myflush - write the buffer back if it was written to, seek to the
   current position; the buffer is empty afterwards */
static void myflush(struct bfile *f, struct Library *dosbase)
{
    if (!FLAGSVALID(f))
        return;
    if (f->flags < 0) {                         /* write it out! */
        Seek(f->fh, f->bufstart, OFFSET_BEGINNING);
        Write(f->fh, f->buf, f->valid);
    }
    f->bufstart += f->pos;
    Seek(f->fh, f->bufstart, OFFSET_BEGINNING);
    FLAGSVALID(f) = 0;
    f->pos = 0;
}

/* mywrite - d1 = file, d2 = buffer, d3 = len; d0 comes back unchanged
   for len 0 */
static ULONG mywrite(ULONG d0, struct bfile *f, const UBYTE *src, ULONG len,
                     struct Library *dosbase)
{
    UBYTE *dst;
    ULONG n;

    if (!len)
        return d0;
    if (len > f->buflen) {                      /* too big for our buffer! */
        myflush(f, dosbase);
        d0 = Write(f->fh, (APTR)src, len);
        if (d0 != (ULONG)-1)
            f->bufstart += d0;
        return d0;
    }
    if ((UWORD)(f->pos + (UWORD)len) > f->buflen)   /* ADD/CMP .w */
        myflush(f, dosbase);
    f->flags = -1;                              /* writeit */
    dst = f->buf + (WORD)f->pos;                /* ADD 26(a3),a0 (adda.w) */
    f->pos += (UWORD)len;
    /* MOVE.l d3,d0:MOVE 26(a3),d0 - len <= buflen, the upper word is 0 */
    d0 = (len & 0xffff0000UL) | f->pos;
    if (f->pos > f->valid)
        f->valid = f->pos;
    n = len;
    do
        *dst++ = *src++;
    while (--n);
    return d0;
}

/* myread - d1 = file, d2 = buffer, d3 = len; d0 comes back unchanged
   for len 0 */
static ULONG myread(ULONG d0, struct bfile *f, UBYTE *dst, ULONG len,
                    struct Library *dosbase)
{
    const UBYTE *src;
    UWORD n;

    if (!len)
        return d0;
    if (len > f->buflen) {                      /* too big for our buffer */
        myflush(f, dosbase);
        d0 = Read(f->fh, dst, len);
        if (d0 != (ULONG)-1)
            f->bufstart += d0;
        return d0;
    }
    if ((UWORD)len > (UWORD)(f->valid - f->pos)) {
        /* not enough valid bytes in buffer, but enough room */
        myflush(f, dosbase);
        d0 = Read(f->fh, f->buf, f->buflen);
        if (d0 == (ULONG)-1)
            return d0;
        f->valid = (UWORD)d0;                   /* bytes read! */
        if ((UWORD)len > (UWORD)d0)             /* CMP d0,d3:BLS */
            len = (len & 0xffff0000UL) | (UWORD)d0;     /* MOVE d0,d3 */
    }
    src = f->buf + (WORD)f->pos;                /* readit */
    f->pos += (UWORD)len;
    if (!len)
        return 0;
    n = (UWORD)len;                             /* DBF d3 */
    do
        *dst++ = *src++;
    while (--n);
    return len;
}

/* myseek - d1 = file, d2 = position, d3 = -1 from the beginning, >= 0
   from the current position; d0 comes back unchanged if the position is
   in the buffer, otherwise the result of dos Seek */
static ULONG myseek(ULONG d0, struct bfile *f, ULONG where, LONG mode,
                    struct Library *dosbase)
{
    ULONG start;

    if ((WORD)mode >= 0) {                      /* TST d3:BMI */
        mode = -1;                              /* to offset beginning */
        where += f->bufstart + (WORD)f->pos;
    }
    start = f->bufstart;                        /* somewhere in buffer? */
    if (where >= start && where < start + (WORD)f->valid) {
        f->pos = (UWORD)(where - start);
        return d0;
    }
    myflush(f, dosbase);                        /* nocando */
    if ((WORD)mode >= 0) {
        where += f->bufstart;
        mode = -1;
    }
    f->bufstart = where;
    f->flags = 1;
    return Seek(f->fh, where, mode);
}

/* ------------------------------------------------------------ routines */

/* Exists - d0 = name: size, -1 for a drawer, 0 if not there */
ULONG BLITZ_ROUTINE doexists(const UBYTE *name BL_REG(d0),
                                      struct Library *dosbase BL_REG(a6))
{
    struct bl_regs r;
    struct FileInfoBlock *fib;
    BPTR key;
    ULONG size;

    key = Lock((STRPTR)name, ACCESS_READ);      /* MOVEQ #-2,d2 */
    if ((LONG)key <= 0)                         /* BLE exfail */
        return 0;
    r = (struct bl_regs){ .d0 = 260, .d1 = 0 };
    fib = (struct FileInfoBlock *)BLITZ_ALIBJSR(TOK_globalloc, r);
    Examine(key, fib);
    UnLock(key);
    size = fib->fib_Size;                       /* 124(a1) */
    if (fib->fib_DirEntryType == 2)             /* 120(a1) */
        size = (ULONG)-1;
    r = (struct bl_regs){ .d0 = 260, .a1 = (ULONG)fib };
    BLITZ_ALIBJSR(TOK_globfree, r);
    return size;
}

/* DosBuffLen - MOVE.l d0,doslen */
static void BLITZ_ROUTINE dodbufflen_c(ULONG n BL_REG(d0))
{
    doslen = n;
}
BLITZ_THUNK(dodbufflen, dodbufflen_c, "d0-d1/a0-a1");

/* WriteMem / ReadMem - a2 = file, d1 = buffer, d2 = len */
void BLITZ_ROUTINE dowritemem(ULONG d0 BL_REG(d0), struct bfile *f BL_REG(a2),
                                       const UBYTE *mem BL_REG(d1), ULONG len BL_REG(d2),
                                       struct Library *dosbase BL_REG(a6))
{
    mywrite(d0, f, mem, len, dosbase);
}

void BLITZ_ROUTINE doreadmem(ULONG d0 BL_REG(d0), struct bfile *f BL_REG(a2),
                                      UBYTE *mem BL_REG(d1), ULONG len BL_REG(d2),
                                      struct Library *dosbase BL_REG(a6))
{
    myread(d0, f, mem, len, dosbase);
}

/* Eof - a2 = file: 0, or -1 if a dos seek one byte further fails */
static ULONG BLITZ_ROUTINE doeof_c(ULONG d0 BL_REG(d0), struct bfile *f BL_REG(a2),
                                   struct Library *dosbase BL_REG(a6))
{
    if (f->pos < f->valid)                      /* CMP .w: BCC slow */
        return 0;
    if (myseek(d0, f, 1, 0, dosbase) + 1 == 0)
        return (ULONG)-1;                       /* atend */
    myseek(d0, f, (ULONG)-1, 0, dosbase);
    return 0;
}
/* the original keeps a0/a1 when the position is in the buffer */
BLITZ_THUNK(doeof, doeof_c, "a0-a1");

/* Lof - a2 = file: dos length (seek to the end and back), at least the
   end of the buffer */
ULONG BLITZ_ROUTINE dolof(struct bfile *f BL_REG(a2),
                                   struct Library *dosbase BL_REG(a6))
{
    ULONG len, end;

    len = Seek(f->fh, 0, OFFSET_END);           /* now at end! */
    len = Seek(f->fh, len, OFFSET_BEGINNING);   /* back to old */
    end = f->valid + f->bufstart;
    if (len < end)                              /* CMP.l: BCC */
        len = end;
    return len;
}

/* Loc - a2 = file: file position of the buffer + position in it */
static ULONG BLITZ_ROUTINE doloc_c(const struct bfile *f BL_REG(a2))
{
    return f->pos + f->bufstart;
}
BLITZ_THUNK(doloc, doloc_c, "d1/a0-a1");

/* FileSeek - a2 = file, d1 = position (from the beginning) */
void BLITZ_ROUTINE dofileseek(ULONG d0 BL_REG(d0), struct bfile *f BL_REG(a2),
                                       ULONG where BL_REG(d1),
                                       struct Library *dosbase BL_REG(a6))
{
    if (myseek(d0, f, where, -1, dosbase) + 1 == 0)
        BLITZ_RUNERR(mess6);                    /* err6 */
}

/* init - d0 = dos base */
static void BLITZ_ROUTINE codeinit_c(struct Library *dos BL_REG(d0))
{
    dosbase_g = dos;
}
BLITZ_THUNK(codeinit, codeinit_c, "d0-d1/a0-a1");

/* dofreefile - close the file (a3), free the buffer and the record
   fields.  Returns a2 as the original leaves it: unchanged, or 0 after
   freeing record fields (see realopen). */
static const ULONG *freefile(struct bfile *f, const ULONG *a2,
                             struct Library *dosbase)
{
    struct bl_regs r;
    struct rec *p;

    if (!f->fh)
        return a2;
    myflush(f, dosbase);
    Close(f->fh);
    f->fh = 0;
    r = (struct bl_regs){ .d0 = f->buflen, .a1 = (ULONG)f->buf };
    f->buf = NULL;                              /* free mydos buffer */
    BLITZ_ALIBJSR(TOK_globfree, r);
    p = f->recs;                                /* BUG(orig): 12(a3) stays */
    if (!p)
        return a2;
    while (p) {
        r = (struct bl_regs){ .d0 = 12, .a1 = (ULONG)p };
        p = p->next;
        BLITZ_ALIBJSR(TOK_globfree, r);
    }
    return NULL;                                /* the loop ends with a2 = 0 */
}

void BLITZ_ROUTINE dofreefile(struct Library *dosbase BL_REG(a6))
{
    freefile((struct bfile *)blitz_string, NULL, dosbase);
}

/* realopen - a1 = file, d1 = name, -4(a2) = its length; mode < 0 old
   file only (ReadFile), > 0 new file only (WriteFile), 0 either
   (OpenFile: old, or new if "object not found") */
static ULONG realopen(WORD mode, struct bfile *f, UBYTE *name, const ULONG *lens,
                      struct Library *dosbase)
{
    struct bl_regs r;
    BPTR fh;

    lens = freefile(f, lens, dosbase);
    __asm__("" : "+a"(lens));                   /* may be 0: read -4(0) */
    name[(WORD)BL_POPL(lens)] = 0;              /* CLR.b 0(a0,d0.w) */

    fh = 0;
    if (mode <= 0) {
        fh = Open((STRPTR)name, MODE_OLDFILE);
        if (!fh && (mode < 0 || IoErr() != ERROR_OBJECT_NOT_FOUND))
            return 0;                           /* openerr */
    }
    if (!fh) {                                  /* newonly */
        fh = Open((STRPTR)name, MODE_NEWFILE);
        if (!fh)
            return 0;
    }
    f->fh = fh;                                 /* file has been opened */
    f->reclen = 0;
    f->unused = 0;
    f->recs = NULL;
    r = (struct bl_regs){ .d0 = doslen, .d1 = MEMF_PUBLIC };
    f->buf = (UBYTE *)BLITZ_ALIBJSR(TOK_globalloc, r);
    f->buflen = (UWORD)doslen;                  /* MOVE doslen+2(pc) */
    FLAGSVALID(f) = 0;
    f->pos = 0;
    f->bufstart = 0;
    return (ULONG)-1;
}

ULONG BLITZ_ROUTINE doopenfile(struct bfile *f BL_REG(a1), UBYTE *name BL_REG(d1),
                                        const ULONG *lens BL_REG(a2),
                                        struct Library *dosbase BL_REG(a6))
{
    return realopen(0, f, name, lens, dosbase);
}

ULONG BLITZ_ROUTINE doreadfile(struct bfile *f BL_REG(a1), UBYTE *name BL_REG(d1),
                                        const ULONG *lens BL_REG(a2),
                                        struct Library *dosbase BL_REG(a6))
{
    return realopen(-1, f, name, lens, dosbase);
}

ULONG BLITZ_ROUTINE dowritefile(struct bfile *f BL_REG(a1), UBYTE *name BL_REG(d1),
                                         const ULONG *lens BL_REG(a2),
                                         struct Library *dosbase BL_REG(a6))
{
    return realopen(1, f, name, lens, dosbase);
}

/* Fields - a3 = file, d7 = number of variables; per variable on the
   argument stack (read downwards from a2): the type (word), then the
   address (long) */
void BLITZ_ROUTINE dorecord(UWORD n BL_REG(d7), const UBYTE *args BL_REG(a2))
{
    struct bfile *f = (struct bfile *)blitz_string;
    const UBYTE *sp = args;
    ULONG reclen = f->reclen;                   /* record len so far */
    struct bl_regs r;

    n--;                                        /* SUBQ #1,d7 */
    do {
        UWORD type = BL_POPW(sp);
        UBYTE *addr = (UBYTE *)BL_POPL(sp);
        ULONG len;
        struct rec *p;

        if (type == 1)
            len = 1;
        else if (type == 2)
            len = 2;
        else if (type != 7)
            len = 4;
        else {
            addr -= 4;                          /* start at length bit */
            len = ((const ULONG *)addr)[-1] + 6;    /* for the length and 0! */
        }
        reclen += len;
        r = (struct bl_regs){ .d0 = 12, .d1 = MEMF_PUBLIC };
        p = (struct rec *)BLITZ_ALIBJSR(TOK_globalloc, r);
        p->next = f->recs;
        f->recs = p;
        p->data = addr;
        p->len = len;
    } while (n-- != 0);                         /* DBF d7 */
    f->reclen = reclen;
}

/* transdata - a3 = file, d1 = record#: seek to record * (word) record
   length, then write (Put) or read (Get) every field */
static void transdata(int put, ULONG d0, UWORD record, struct Library *dosbase)
{
    struct bfile *f = (struct bfile *)blitz_string;
    struct rec *p;

    /* seekrec: MULU 6(a3),d1 */
    if (myseek(d0, f, (ULONG)record * (UWORD)f->reclen, -1, dosbase) + 1 == 0) {
        BLITZ_RUNERR(mess3);                    /* err3 */
        return;
    }
    for (p = f->recs; p; p = p->next) {
        if (put)
            d0 = mywrite((ULONG)p, f, p->data, p->len, dosbase);
        else
            d0 = myread((ULONG)p, f, p->data, p->len, dosbase);
        if ((LONG)d0 < 0) {                     /* TST.l d0:BMI err7 */
            BLITZ_RUNERR(mess7);
            return;
        }
    }
}

void BLITZ_ROUTINE doput(ULONG d0 BL_REG(d0), UWORD record BL_REG(d1),
                                  struct Library *dosbase BL_REG(a6))
{
    transdata(1, d0, record, dosbase);
}

void BLITZ_ROUTINE doget(ULONG d0 BL_REG(d0), UWORD record BL_REG(d1),
                                  struct Library *dosbase BL_REG(a6))
{
    transdata(0, d0, record, dosbase);
}

/* KillFile - JMP _DeleteFile(a6) */
void BLITZ_ROUTINE dokillfile(const UBYTE *name BL_REG(d0),
                                       struct Library *dosbase BL_REG(a6))
{
    DeleteFile((STRPTR)name);
}

/* ------------------------------------------------- the I/O handlers */

void inputhandler(void), outputhandler(void);

/* FileInput / FileOutput - a3 = file; set inputoutputlib's handler,
   which keeps d1/a1 */
static void BLITZ_ROUTINE dofileinput_c(void)
{
    struct bl_regs r = { .a0 = (ULONG)inputhandler };

    infile = (struct bfile *)blitz_string;
    BLITZ_ALIBJSR(TOK_setin, r);
}
BLITZ_THUNK(dofileinput, dofileinput_c, "d1/a1");

static void BLITZ_ROUTINE dofileoutput_c(void)
{
    struct bl_regs r = { .a0 = (ULONG)outputhandler };

    outfile = (struct bfile *)blitz_string;
    BLITZ_ALIBJSR(TOK_setout, r);
}
BLITZ_THUNK(dofileoutput, dofileoutput_c, "d1/a1");

/* inputhandler - one character: d0 = 0..255, -1 at the end */
static ULONG BLITZ_ROUTINE inchar_c(void)
{
    struct bfile *f = infile;
    ULONG d0;

    if (!f->fh) {
        BLITZ_RUNERR(mess1);                    /* err1 */
        return 0;
    }
    d0 = myread(0, f, &temp, 1, dosbase_g);
    if (d0 == (ULONG)-1) {
        BLITZ_RUNERR(mess7);                    /* err7 */
        return 0;
    }
    return d0 ? temp : (ULONG)-1;
}

/* inhandle2 - a0 = buffer, d0 = bytes max: read, cut behind the first
   CR or LF (seek back to the character after it); d0 = the characters
   in front of it, or all characters read */
static ULONG BLITZ_ROUTINE instring_c(UBYTE *buf BL_REG(a0), ULONG max BL_REG(d0))
{
    struct bfile *f = infile;
    ULONG got;
    LONG i, n;

    if (!f->fh) {
        BLITZ_RUNERR(mess1);                    /* err1 */
        return 0;
    }
    got = myread(max, f, buf, max, dosbase_g);
    if ((LONG)got < 0) {                        /* TST.l d0:BMI err7 */
        BLITZ_RUNERR(mess7);
        return 0;
    }
    n = got;
    for (i = 0; --n >= 0; i++) {                /* SUBQ.l #1,d2:BMI done */
        if (buf[i] == 10 || buf[i] == 13) {     /* found */
            LONG back = i - got + 1;
            if (back)
                myseek(got, f, back, 0, dosbase_g);
            return i;
        }
    }
    return got;
}

/* outputhandler - one character in d0 */
static ULONG BLITZ_ROUTINE outchar_c(UBYTE c BL_REG(d0))
{
    struct bfile *f = outfile;
    ULONG d0;

    if (!f->fh) {
        BLITZ_RUNERR(mess2);                    /* err2 */
        return 0;
    }
    temp = c;
    d0 = mywrite(c, f, &temp, 1, dosbase_g);
    if (d0 + 1 == 0)
        BLITZ_RUNERR(mess7);                    /* err7 */
    return d0;
}

/* outhandle2 - a0 = string, d0 = length */
static ULONG BLITZ_ROUTINE outstring_c(const UBYTE *s BL_REG(a0), ULONG len BL_REG(d0))
{
    struct bfile *f = outfile;
    ULONG d0;

    if (!f->fh) {
        BLITZ_RUNERR(mess2);                    /* err2 */
        return 0;
    }
    d0 = mywrite(len, f, s, len, dosbase_g);
    if (d0 + 1 == 0)
        BLITZ_RUNERR(mess7);                    /* err7 */
    return d0;
}

/* outhandle3 - a0 = 0-terminated string; keeps d0 */
static void BLITZ_ROUTINE outcstring_c(const UBYTE *s BL_REG(a0))
{
    struct bfile *f;
    ULONG len = 0;

    while (s[len])
        len++;
    if (!len)
        return;
    f = outfile;
    if (!f->fh) {
        BLITZ_RUNERR(mess2);                    /* err2 */
        return;
    }
    if (mywrite(0, f, s, len, dosbase_g) + 1 == 0)
        BLITZ_RUNERR(mess7);                    /* err7 */
}

/* The handlers with their BRA entries in front (inputoutputlib calls
   -8/-4/0 of the address it was given).  The registers each one keeps
   are those of the original (MOVEM in its first line); GCC keeps
   d2-d7/a2-a6 anyway. */
BLITZ_THUNK(inhandle2, instring_c, "d1/a1");
BLITZ_THUNK(outhandle2, outstring_c, "d1/a1");
BLITZ_THUNK(outhandle3, outcstring_c, "d0-d1/a1");
__asm__("   .text\n   .even\n"
        "   bra.w   _inhandle2\n"               /* -4 */
        "   .globl  _inputhandler\n"
        "_inputhandler:\n"                      /*  0 */
        "   movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _inchar_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n"
        "   bra.w   _outhandle3\n"              /* -8 */
        "   bra.w   _outhandle2\n"              /* -4 */
        "   .globl  _outputhandler\n"
        "_outputhandler:\n"                     /*  0 */
        "   movem.l d1/a0-a1,-(sp)\n"
        "   jsr     _outchar_c\n"
        "   movem.l (sp)+,d1/a0-a1\n"
        "   rts\n");

/* ------------------------------------------- error checks (debugger only) */

BLITZ_ERRCHK_MESSAGE(mess4, "Non-Existant File");
BLITZ_ERRCHK_MESSAGE(mess5, "No Record Set");
BLITZ_ERRCHK_MESSAGE(mess8, "Illegal Dos Buffer Length");

/* racchk - Put/Get: a3 = file open and with fields */
static void BLITZ_ERRCHK racchk_c(void)
{
    const struct bfile *f = (const struct bfile *)blitz_string;

    if (!f->fh)
        BLITZ_RUNERR(mess4);
    else if (!f->recs)
        BLITZ_RUNERR(mess5);
}

/* fchk - a2 = file open */
static void BLITZ_ERRCHK fchk_c(const struct bfile *f BL_REG(a2))
{
    if (!f->fh)
        BLITZ_RUNERR(mess4);
}

/* dbchk - DosBuffLen at most 32767 (unsigned) */
static void BLITZ_ERRCHK dbchk_c(ULONG n BL_REG(d0))
{
    if (n > 32767)
        BLITZ_RUNERR(mess8);
}

/* the checks only test: they keep every register */
BLITZ_ERRCHK_THUNK(racchk, racchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(fchk, fchk_c, "d0-d1/a0-a1");
BLITZ_ERRCHK_THUNK(dbchk, dbchk_c, "d0-d1/a0-a1");
