/*
 * amosfunc.h - RIAmosFuncLib (userlib 75) in the RWE tests that load it
 * (riamosfunclib, rigfxlib, ripacklib; header only, after sysload.h).
 *
 *   AMOS_E_...          indexes in its token list (token = (75 << 7) + 1 +
 *                       index; the same in the BSS2 and the AmiBlitz3 binary)
 *   amosfunc_fixjsrs()  after load_sets(): clear bit 0 of ALibJsr targets
 *
 * Several of its routines are Amiga mode only (`!subs {code+1,...}`:
 * bit 0 of the address, blitz2.s).  A program built by the compiler never
 * jumps there directly: tokecode (asmlibs.c, blitz2.s:6047) makes a stub
 * for such a form (cutejsr).  sysload's loader resolves an ALibJsr
 * placeholder to the address as it is in the table, bit 0 included - a
 * JSR to an odd address (address error).  amosfunc_fixjsrs clears that
 * bit in the operand of every resolved `JSR abs.l` (both sets, every
 * library), which is where the stub would jump.
 */
#ifndef AMOSFUNC_H
#define AMOSFUNC_H

/* entries of the token list (the ones the tests call directly) */
#define AMOS_E_LENGTH    17
#define AMOS_E_NEXTBANK  21
#define AMOS_E_BANKLIMIT 35
#define AMOS_E_FINIT     37
#define AMOS_E_INIT      38

static __attribute__((unused)) void amosfunc_fixjsrs(void)
{
    int s, i;
    ULONG r;

    for (s = 0; s < 2; s++)
        for (i = 0; i < sets[s].n; i++) {
            struct lib *l = &sets[s].lib[i];
            for (r = 0; r < l->nrelocs; r++) {
                UBYTE *at = l->code + l->relocs[r];
                if (l->relocs[r] >= 2 && W(at - 2) == 0x4eb9 && (L(at) & 1))
                    *(ULONG *)at &= ~1UL;
            }
        }
    CacheClearU();
}

#endif
