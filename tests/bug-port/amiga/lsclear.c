/*
 * lsclear - does LoadSeg clear the tail of CODE/DATA hunks beyond the
 * stored block?  (the test of the port defect #963)
 *
 *   lsclear [-gen] FILE...
 *
 * Kickstart 1.x's LoadSeg clears only HUNK_BSS; the part of a CODE/DATA
 * hunk between the end of its stored block and the size in the hunk
 * header is left as the memory was.  A program linked with -fbaserel and
 * -nostartfiles (ted, ab3cli) has its .bss there and never clears it.
 *
 * For every FILE the program dirties all free memory (chip and any) with
 * $AA, parses the hunk sizes of the load file itself, loads it with
 * LoadSeg and counts the non-zero bytes of every hunk beyond its stored
 * block (a HUNK_BSS from its start).  Output, one line each:
 *
 *   dirty chip=H any=H probe_chip=H probe_any=H
 *       bytes dirtied; non-zero bytes of a fresh AllocMem (must be > 0,
 *       else the run proves nothing and counts as FAIL)
 *   hunk N CODE|DATA|BSS  size=H stored=H tail_nonzero=H [first@H=HH]
 *   result NAME PASS|FAIL tail_nonzero=H
 *   lsclear: PASS|FAIL                   (PASS = every file PASS)
 *
 * -gen also writes three small load files to RAM: and checks them, for
 * the self test of the checker: RAM:t_tail (CODE 16 bytes, DATA 1024
 * bytes of which 16 are stored: the tail is not in the file), RAM:t_zero
 * (the same with the tail stored as zeros) and RAM:t_bss (CODE, HUNK_BSS
 * 1024 bytes).  Return code 0 = PASS, 10 = FAIL, 20 = usage or error.
 * No libc, Kickstart 1.2 calls only.
 */
__asm__("  .text\n  .globl __start\n__start:\n  move.l d0,-(sp)\n  move.l a0,-(sp)\n"
        "  jsr _run\n  addq.l #8,sp\n  rts\n");

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
static BPTR out;

static void puts_(const char *s)
{
	LONG n = 0;
	while (s[n])
		n++;
	Write(out, (APTR)s, n);
}

static void hex(ULONG v, int digits)
{
	char b[9];
	int i;
	for (i = digits - 1; i >= 0; i--) {
		b[i] = "0123456789abcdef"[v & 15];
		v >>= 4;
	}
	b[digits] = 0;
	puts_(b);
}

static void kv(const char *k, ULONG v)
{
	puts_(k);
	hex(v, 8);
}

struct chunk {
	struct chunk *next;
	ULONG size;
};

/* fills all free memory of the type with $AA, then frees it again */
static ULONG dirty(ULONG req)
{
	static const ULONG sizes[] = { 262144, 65536, 4096, 256, 16 };
	struct chunk *list = 0, *c;
	ULONG total = 0;
	int i;
	for (i = 0; i < 5; i++) {
		ULONG s = sizes[i];
		while ((c = AllocMem(s, req)) != 0) {
			ULONG *p = (ULONG *)c, *e = (ULONG *)((UBYTE *)c + s);
			while (p < e)
				*p++ = 0xAAAAAAAA;
			c->next = list;
			c->size = s;
			list = c;
			total += s;
		}
	}
	while (list) {
		c = list->next;
		FreeMem(list, list->size);
		list = c;
	}
	return total;
}

/* non-zero bytes of a fresh block (the memory must have come back dirty) */
static ULONG probe(ULONG req)
{
	UBYTE *m = AllocMem(4104, req);
	ULONG i, nz = 0;
	if (!m)
		return 0;
	for (i = 8; i < 4104; i++)
		if (m[i])
			nz++;
	FreeMem(m, 4104);
	return nz;
}

#define MAXH 16
static ULONG hsize[MAXH], hstored[MAXH], htype[MAXH];
static int nh;

/* the header sizes and stored block lengths of a load file */
static int parse(UBYTE *buf, ULONG len)
{
	ULONG *p = (ULONG *)buf, *e = (ULONG *)(buf + len), first, last, i, t;
	int h;
	if (len < 32 || *p++ != 0x3f3)
		return 0;
	while ((t = *p++) != 0)
		p += t;
	p++;
	first = *p++;
	last = *p++;
	nh = last - first + 1;
	if (nh < 1 || nh > MAXH)
		return 0;
	for (i = 0; i < (ULONG)nh; i++) {
		t = *p++;
		hsize[i] = (t & 0x3fffffff) * 4;
		if ((t >> 30) == 3)
			p++;
		hstored[i] = 0;
		htype[i] = 0;
	}
	h = 0;
	while (p < e && h < nh) {
		t = *p++ & 0x3fffffff;
		switch (t) {
		case 0x3e9:
		case 0x3ea:
			htype[h] = t;
			hstored[h] = *p * 4;
			p += *p + 1;
			break;
		case 0x3eb:
			htype[h] = t;
			hstored[h] = 0;
			p++;
			break;
		case 0x3ec: case 0x3ed: case 0x3ee: case 0x3f7: case 0x3f8:
		case 0x3f0:
			while ((t = *p++) != 0)
				p += t + 1;
			break;
		case 0x3f1: case 0x3e8:
			p += *p + 1;
			break;
		case 0x3f2:
			h++;
			break;
		default:
			puts_("unknown hunk type ");
			hex(t, 8);
			puts_("\n");
			return 0;
		}
	}
	return h == nh;
}

/* 1 = PASS, 0 = FAIL, -1 = error */
static int check(char *name)
{
	BPTR fh = Open(name, MODE_OLDFILE), seg, s;
	LONG len;
	UBYTE *buf;
	int h, ok;
	ULONG total = 0;
	if (!fh) {
		puts_("cannot open ");
		puts_(name);
		puts_("\n");
		return -1;
	}
	Seek(fh, 0, OFFSET_END);
	len = Seek(fh, 0, OFFSET_BEGINNING);
	buf = AllocMem(len, MEMF_PUBLIC);
	if (!buf) {
		Close(fh);
		return -1;
	}
	Read(fh, buf, len);
	Close(fh);
	h = parse(buf, len);
	FreeMem(buf, len);
	if (!h) {
		puts_("parse failed: ");
		puts_(name);
		puts_("\n");
		return -1;
	}
	seg = LoadSeg(name);
	if (!seg) {
		puts_("LoadSeg failed: ");
		puts_(name);
		puts_("\n");
		return -1;
	}
	for (s = seg, h = 0; s && h < nh; s = *(BPTR *)BADDR(s), h++) {
		ULONG *base = (ULONG *)BADDR(s);
		UBYTE *d = (UBYTE *)(base + 1);
		ULONG nz = 0, firstnz = 0, firstval = 0, i;
		for (i = hstored[h]; i < hsize[h]; i++)
			if (d[i]) {
				if (!nz) {
					firstnz = i;
					firstval = d[i];
				}
				nz++;
			}
		total += nz;
		puts_("  hunk ");
		hex(h, 1);
		puts_(htype[h] == 0x3e9 ? " CODE" : htype[h] == 0x3ea ? " DATA"
		      : htype[h] == 0x3eb ? " BSS " : " ????");
		kv(" size=", hsize[h]);
		kv(" stored=", hstored[h]);
		kv(" tail_nonzero=", nz);
		if (nz) {
			kv(" first@", firstnz);
			puts_("=");
			hex(firstval, 2);
		}
		puts_("\n");
	}
	UnLoadSeg(seg);
	ok = total == 0;
	puts_("result ");
	puts_(name);
	puts_(ok ? " PASS" : " FAIL");
	kv(" tail_nonzero=", total);
	puts_("\n");
	return ok;
}

static void wl(BPTR fh, ULONG v)
{
	Write(fh, &v, 4);
}

/* kind 0: tail not stored, 1: tail stored as zeros, 2: HUNK_BSS */
static void gen(char *name, int kind)
{
	BPTR fh = Open(name, MODE_NEWFILE);
	int i;
	if (!fh) {
		puts_("cannot write ");
		puts_(name);
		puts_("\n");
		return;
	}
	wl(fh, 0x3f3); wl(fh, 0); wl(fh, 2); wl(fh, 0); wl(fh, 1);
	wl(fh, 4);
	wl(fh, 256);
	wl(fh, 0x3e9); wl(fh, 4); wl(fh, 0x70004e75);
	for (i = 0; i < 3; i++)
		wl(fh, 0x11111111);
	wl(fh, 0x3f2);
	if (kind == 2) {
		wl(fh, 0x3eb); wl(fh, 256);
	} else {
		wl(fh, 0x3ea);
		wl(fh, kind == 1 ? 256 : 4);
		for (i = 0; i < (kind == 1 ? 256 : 4); i++)
			wl(fh, kind == 1 ? 0 : 0x22222222);
	}
	wl(fh, 0x3f2);
	Close(fh);
}

#define MAXF 16

int run(char *args, LONG len)
{
	static char names[MAXF][128];
	int i, n = 0, nf = 0, fails = 0, genmode = 0;
	SysBase = *(struct ExecBase **)4;
	DOSBase = (struct DosLibrary *)OpenLibrary((STRPTR)"dos.library", 0);
	if (!DOSBase)
		return 20;
	out = Output();
	for (i = 0; i <= len; i++) {
		char ch = i < len ? args[i] : ' ';
		if (ch == ' ' || ch == '\n' || ch == '\t') {
			if (n && nf < MAXF) {
				names[nf][n] = 0;
				if (names[nf][0] == '-' && names[nf][1] == 'g')
					genmode = 1;
				else
					nf++;
			}
			n = 0;
		} else if (n < 127 && nf < MAXF)
			names[nf][n++] = ch;
	}
	if (genmode) {
		gen("RAM:t_tail", 0);
		gen("RAM:t_zero", 1);
		gen("RAM:t_bss", 2);
		names[nf][0] = 0;
		for (i = 0; i < 3 && nf < MAXF; i++) {
			static const char *g[] = { "RAM:t_tail", "RAM:t_zero", "RAM:t_bss" };
			int k = 0;
			while ((names[nf][k] = g[i][k]) != 0)
				k++;
			nf++;
		}
	}
	if (!nf) {
		puts_("usage: lsclear [-gen] FILE...\n");
		CloseLibrary((struct Library *)DOSBase);
		return 20;
	}
	for (i = 0; i < nf; i++) {
		ULONG c = dirty(MEMF_CHIP), a = dirty(MEMF_ANY);
		ULONG pc = probe(MEMF_CHIP), pa = probe(MEMF_ANY);
		puts_("dirty");
		kv(" chip=", c);
		kv(" any=", a);
		kv(" probe_chip=", pc);
		kv(" probe_any=", pa);
		puts_("\n");
		if (!pc || !pa) {
			puts_("memory not dirty: inconclusive\n");
			fails++;
		} else if (check(names[i]) != 1)
			fails++;
	}
	puts_(fails ? "lsclear: FAIL\n" : "lsclear: PASS\n");
	CloseLibrary((struct Library *)DOSBase);
	return fails ? 10 : 0;
}
