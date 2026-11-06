/*
 * tedprobe - gate 0 of the TED A/B tests (bb2 ted-c/notes/PHASE3-PLAN.md
 * 3.3): what TED will find when it clones the Workbench screen.
 *
 *   tedprobe OUTFILE [KEYMAP]   "key value" lines to stdout and OUTFILE
 *
 * TED's calcscreen reads GetScreenData(84 bytes, WBENCHSCREEN): Width,
 * Height, ViewPort.Modes; it opens its screen with SA_LikeWorkbench and
 * SA_Depth 2 and then draws into BitMap->Planes[0/1] with the CPU and the
 * blitter.  That only works if the Workbench bitmap is planar chip memory
 * (no RTG) and the file requester needs a screen below 605 lines.
 *
 * KEYMAP (e.g. d): first make that keymap the default one, as IPrefs
 * does for the keymap of the input prefs - the drivers type with MapANSI
 * and TED converts with RawKeyConvert, both with the default keymap, and
 * the scripts' umlauts need the German one (28-long); the session sets it
 * instead of depending on the input prefs of the hardfile.
 *
 * Last line: "gate0 OK" (return 0) or "gate0 FAIL reason..." (return 5,
 * WARN - the session script skips the rest).  Everything else is
 * information for the runner (tests/ted-ab/runner, tedab), which also
 * checks the expected geometry.
 */
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/gfxbase.h>
#include <graphics/displayinfo.h>
#include <devices/keymap.h>
#include <devices/keyboard.h>
#include <exec/io.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include "tdout.h"

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *KeymapBase;

static int fails;
static void fail(const char *why)
{
    o_str("gate0 check failed: ");
    o_str(why);
    o_line();
    fails++;
}

static int loaded(const char *name)
{
    int r;
    Forbid();
    r = FindName(&SysBase->LibList, (CONST_STRPTR)name) != NULL;
    Permit();
    return r;
}

static int exists(const char *path)
{
    struct Process *pr = (struct Process *)FindTask(NULL);
    APTR old = pr->pr_WindowPtr;
    BPTR l;
    pr->pr_WindowPtr = (APTR)-1;            /* no "insert volume" requester */
    l = Lock((CONST_STRPTR)path, SHARED_LOCK);
    pr->pr_WindowPtr = old;
    if (l) UnLock(l);
    return l != 0;
}

/* the keymap NAME as the default keymap: the one of keymap.resource's
   list, else DEVS:Keymaps/NAME loaded (its first hunk starts with the
   KeyMapNode) and added to the list, where it stays (as IPrefs leaves
   it); 0 if there is none */
static int setkeymap(const char *name)
{
    struct KeyMapResource *kr = (struct KeyMapResource *)OpenResource((CONST_STRPTR)"keymap.resource");
    struct KeyMapNode *kn;
    char path[64];
    BPTR seg;

    if (!kr || strlen(name) > sizeof path - 14)
        return 0;
    Forbid();
    kn = (struct KeyMapNode *)FindName(&kr->kr_List, (CONST_STRPTR)name);
    Permit();
    if (!kn) {
        strcpy(path, "DEVS:Keymaps/");
        strcat(path, name);
        if (!exists(path) || !(seg = LoadSeg((CONST_STRPTR)path)))
            return 0;
        kn = (struct KeyMapNode *)((BPTR *)BADDR(seg) + 1);
        Forbid();
        AddHead(&kr->kr_List, &kn->kn_Node);
        Permit();
    }
    SetKeyMapDefault(&kn->kn_KeyMap);
    return 1;
}

static const char usage[] =
    "usage: tedprobe OUTFILE [KEYMAP]\n"
    "  gate 0 of the TED A/B tests: what TED finds when it clones the Workbench;\n"
    "  KEYMAP: make it the default keymap first\n";

int main(int argc, char **argv)
{
    UBYTE gsd[sizeof(struct Screen)];
    struct Screen *wb;
    struct Task *me = FindTask(NULL);
    struct CommandLineInterface *cli = Cli();

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        Write(Output(), (APTR)usage, sizeof usage - 1);
        return 0;
    }
    if (argc != 2 && argc != 3) {
        o_str("usage: tedprobe OUTFILE [KEYMAP]");
        o_line();
        return 20;
    }
    o_file = Open((CONST_STRPTR)argv[1], MODE_NEWFILE);
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library", 39);
    GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR)"graphics.library", 39);
    KeymapBase = OpenLibrary((CONST_STRPTR)"keymap.library", 37);
    o_ks("magic", "TEDPROBE1");
    if (!IntuitionBase || !GfxBase || !KeymapBase) {
        fail("libraries (intuition/graphics V39, keymap V37) missing");
        goto done;
    }

    /* system */
    o_kv("exec_version", SysBase->LibNode.lib_Version);
    o_kv("exec_revision", SysBase->LibNode.lib_Revision);
    o_kv("intuition_version", IntuitionBase->LibNode.lib_Version);
    o_kv("graphics_version", GfxBase->LibNode.lib_Version);
    o_kx("attnflags", SysBase->AttnFlags);
    o_kx("chiprevbits0", GfxBase->ChipRevBits0);
    o_kx("displayflags", GfxBase->DisplayFlags);
    o_kv("rtg_picasso96_loaded", loaded("Picasso96API.library"));
    o_kv("rtg_cybergraphics_loaded", loaded("cybergraphics.library"));

    /* exactly TED's call (calcscreen: 84 bytes) */
    memset(gsd, 0, sizeof gsd);
    if (!GetScreenData(gsd, 84, WBENCHSCREEN, NULL)) {
        fail("GetScreenData(WBENCHSCREEN) failed");
    } else {
        struct Screen *s = (struct Screen *)gsd;
        UWORD w = s->Width & 0xFFF0, h = s->Height;
        /* TED's geometry (calcscreen, fonthite 8 and 6, mousecols 0) */
        UWORD rows8 = (UWORD)((h - 20) / 8 - 1), rows6 = (UWORD)((h - 20) / 6 - 1);
        o_kv("gsd_width", s->Width);
        o_kv("gsd_height", s->Height);
        o_kx("gsd_modes", s->ViewPort.Modes);
        o_kv("ted_width", w);
        o_kv("ted_edcols", w / 8 - 2);
        o_kv("ted_edrows_f8", rows8);
        o_kv("ted_height_f8", rows8 * 8 + 8 + 8 + 1 + 12);
        o_kv("ted_edrows_f6", rows6);
        o_kv("ted_height_f6", rows6 * 6 + 6 + 8 + 1 + 12);
        if (rows8 * 8 + 29 > 604 || rows6 * 6 + 27 > 604)
            fail("TED's screen would be higher than 604 lines (file requester overflow)");
        if (w < 640)
            fail("Workbench narrower than 640 pixels");
    }

    /* the real Workbench screen */
    wb = LockPubScreen((CONST_STRPTR)"Workbench");
    if (!wb) {
        fail("no Workbench screen");
    } else {
        struct BitMap *bm = wb->RastPort.BitMap;
        ULONG id = GetVPModeID(&wb->ViewPort);
        struct NameInfo ni;
        ULONG t0 = TypeOfMem(bm->Planes[0]);

        o_kv("wb_width", wb->Width);
        o_kv("wb_height", wb->Height);
        o_kx("wb_modeid", id);
        if (GetDisplayInfoData(NULL, (UBYTE *)&ni, sizeof ni, DTAG_NAME, id) > 0)
            o_ks("wb_modename", (const char *)ni.Name);
        o_kv("wb_depth", GetBitMapAttr(bm, BMA_DEPTH));
        o_kx("wb_bmflags", GetBitMapAttr(bm, BMA_FLAGS));
        o_kv("wb_bm_depth_field", bm->Depth);
        o_kv("wb_bytesperrow", bm->BytesPerRow);
        o_kv("wb_rows", bm->Rows);
        o_kx("wb_plane0", (ULONG)bm->Planes[0]);
        o_kx("wb_plane1", (ULONG)bm->Planes[1]);
        o_kx("wb_plane0_memtype", t0);
        o_kv("wb_plane0_chip", (t0 & MEMF_CHIP) != 0);
        o_kv("wb_interleaved", bm->Depth > 1 &&
             (ULONG)bm->Planes[1] - (ULONG)bm->Planes[0] == (ULONG)((wb->Width + 15) / 16 * 2));
        o_ks("wb_title", (const char *)wb->Title);
        if (!(t0 & MEMF_CHIP))
            fail("Workbench bitplanes not in chip memory (RTG?)");
        if (bm->Depth < 2)
            fail("Workbench depth below 2");
        UnlockPubScreen(NULL, wb);
    }

    /* memory */
    o_kv("chip_free", AvailMem(MEMF_CHIP));
    o_kv("chip_largest", AvailMem(MEMF_CHIP | MEMF_LARGEST));
    o_kv("fast_free", AvailMem(MEMF_FAST));
    o_kv("any_free", AvailMem(MEMF_ANY));

    /* TED's option file: loaddefs reads l:blitzeditor.opts */
    o_kv("sys_l_blitzeditor_opts", exists("SYS:L/blitzeditor.opts"));
    o_kv("l_blitzeditor_opts", exists("L:blitzeditor.opts"));

    /* stacks: the runner's Stack command and this process */
    if (cli) o_kv("cli_defaultstack", cli->cli_DefaultStack * 4);
    o_kv("task_stack", (ULONG)me->tc_SPUpper - (ULONG)me->tc_SPLower);

    /* keymap (teddrive types with MapANSI, TED converts with
       RawKeyConvert of console.device - both use the default keymap) */
    if (argc == 3 && !setkeymap(argv[2]))
        fail("keymap not found (keymap.resource, DEVS:Keymaps)");
    {
        struct KeyMap *km = AskKeyMapDefault();
        struct KeyMapNode *kn = km ? (struct KeyMapNode *)((UBYTE *)km - sizeof(struct Node)) : NULL;
        static const char probe[] = "Ab1 ;$\"_\n";
        char ev[40];
        LONG n, i;
        o_ks("keymap", kn ? (const char *)kn->kn_Node.ln_Name : "(none)");
        n = MapANSI((CONST_STRPTR)probe, sizeof probe - 1, (STRPTR)ev, sizeof ev, NULL);
        o_str("mapansi \"Ab1 ;$\\\"_\\n\" ->");
        for (i = 0; i < n; i++) {
            o_str(" ");
            o_hex((UBYTE)ev[2 * i], 2);
            o_str("/");
            o_hex((UBYTE)ev[2 * i + 1], 2);
        }
        o_str(" (");
        o_dec(n);
        o_str(" events)");
        o_line();
        /* information only: `type` in teddrive refuses such characters */
        o_kv("mapansi_onekey", n == (LONG)sizeof probe - 1);
    }

    /* no key may be held: the main windows of TED and teddrivetest are
       active, real key events (a key stuck in the emulator, auto-repeat)
       would mix with the synthetic input (stage 1, session 2b: 's' with
       IEQUALIFIER_REPEAT reached gate D) */
    {
        struct MsgPort *kp = CreateMsgPort();
        struct IOStdReq *kio = kp ? (struct IOStdReq *)CreateIORequest(kp, sizeof *kio) : NULL;
        UBYTE matrix[16];
        int k, down = 0;
        if (kio && !OpenDevice((CONST_STRPTR)"keyboard.device", 0, (struct IORequest *)kio, 0)) {
            memset(matrix, 0, sizeof matrix);
            kio->io_Command = KBD_READMATRIX;
            kio->io_Data = matrix;
            kio->io_Length = 13;            /* raw keys $00-$67 */
            DoIO((struct IORequest *)kio);
            o_str("keys_down");
            for (k = 0; k < 0x68; k++)
                if (matrix[k >> 3] & (1 << (k & 7))) {
                    o_str(" ");
                    o_hex(k, 2);
                    down++;
                }
            o_line();
            CloseDevice((struct IORequest *)kio);
            if (down)
                fail("keys held down in the emulator (keyboard matrix)");
        } else
            fail("keyboard.device");
        if (kio) DeleteIORequest((struct IORequest *)kio);
        if (kp) DeleteMsgPort(kp);
    }

done:
    o_str(fails ? "gate0 FAIL" : "gate0 OK");
    o_line();
    if (KeymapBase) CloseLibrary(KeymapBase);
    if (GfxBase) CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (o_file) Close(o_file);
    return fails ? 5 : 0;
}
