/*
 * amosfunc.h - RIAmosFuncLib (userlib 75) in the RWE tests that load it
 * (riamosfunclib, rigfxlib, ripacklib; header only, after sysload.h).
 *
 *   AMOS_E_...          indexes in its token list (token = (75 << 7) + 1 +
 *                       index; the same in the BSS2 and the AmiBlitz3 binary)
 * (The Amiga-mode-only bit 0 of routine addresses is cleared by the
 * loader, sysload.h link_set.)
 */
#ifndef AMOSFUNC_H
#define AMOSFUNC_H

/* entries of the token list (the ones the tests call directly) */
#define AMOS_E_LENGTH    17
#define AMOS_E_NEXTBANK  21
#define AMOS_E_BANKLIMIT 35
#define AMOS_E_FINIT     37
#define AMOS_E_INIT      38

#endif
