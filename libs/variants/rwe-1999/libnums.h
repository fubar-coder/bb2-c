/* libnums.h of the variant rwe-1999: bb2-2.10 and the userlibs of the
   library sources Red When Excited released in September 1999
   (archive/aminet/amibl2src/rwelibsrc, docs/rwelibs/README.md).  The
   numbers bb2-2.10 does not have: from AmiBlitz3's sorted_libnums.ab3 or
   the number of the CD binary the source builds byte-identically to
   (docs/rwelibs/README.md 3.1). */
#ifndef VARIANT_LIBNUMS_H
#define VARIANT_LIBNUMS_H
#include "../bb2-2.10/libnums.h"

#define LIB_ricopperfx              11   /* BSS2 RICOPPERFXLIB */
/* RIDosLib keeps its number 33, which bb2-2.10 reserves for literallib
   (docs/token-table.md): the RWE variants have no literal library and no
   literal characters (ACIDLIBS_WITHOUT_LITERAL, libs/acidlibs) */
#undef LIB_literallib
#define LIB_ridoslib                33   /* BSS2 RIDOSLIB; AmiBlitz3 #ridoslib */
#define LIB_rinonvolatilelib        35   /* BSS2 RINONVOLATILELIB */
#define LIB_rifastseriallib         36   /* BSS2 RIFASTSERIALLIB */
#define LIB_ridebuglib              51   /* OL RIDEBUGLIB (Leading Edge Software) */
#define LIB_ristonecrackerlib       17   /* BSS2 RISTONECRACKERLIB; RWE's list */
#define LIB_risoundlib              45   /* BSS2 RISOUNDLIB; RWE's list */
/* the libraries whose reference is the reference build of the source
   (docs/rwelibs/README.md 3.2 "Source"), numbers as their binaries */
#define LIB_rilinkedlistlib         12   /* OL RILISTLIB; AmiBlitz3 */
#define LIB_risortlib               13   /* BSS2 RISORTLIB; AmiBlitz3 */
#define LIB_ricompactdisclib        14   /* BSS2 RICOMPACTDISKLIB; AmiBlitz3 */
#define LIB_ristcdecrunchlib        18   /* BSS2 RISTCDECRUNCHLIB; AmiBlitz3 */
#define LIB_ridatatypeslib          25   /* BSS2 RIDATATYPESLIB; AmiBlitz3 */
#define LIB_rilvcallhooklib         27   /* AmiBlitz3 RILVCallHook */
#define LIB_rigtmenulib             37   /* BSS2 RIGTMENULIB; AmiBlitz3 */
#define LIB_rishapeslib             46   /* BSS2 RISHAPESLIB; AmiBlitz3 */
#define LIB_rimisclib               47   /* BSS2 RIMISCLIB; AmiBlitz3 */
#define LIB_ridisasmlib             50   /* AmiBlitz3 RIDisAsm */
#define LIB_ciatrklib               56   /* = #neilsciatrklib of the CD's libnums */
#define LIB_ritrackerlib            58   /* RWE's list (docs/rwelibs/README.md 3.3), no binary */
#define LIB_riLESDEBUGlib           49   /* BUM RILESDEBUGLIB; AmiBlitz3 RILESDEBUGLib
                                            (the source's case: Blitz constants
                                            are case-sensitive) */
/* stage D: no binary; numbers from RWE's list (docs/rwelibs/README.md
   3.3) under the name the source uses, or the literal number of the
   source if it is free in this set, or a slot RWE's list reserved */
#define LIB_riregistrylib            3   /* RWE's list */
#define LIB_rimapshapeslib           6   /* RWE's list */
#define LIB_riarrayslib              7   /* RWE's list */
#define LIB_rimapeditorlib           8   /* RWE's list; c2plib's literal 8 left out */
#define LIB_riwbspritelib           10   /* literal in WBSprite-library2 */
#define LIB_riintuilib              15   /* RWE's list */
#define LIB_popupwinlib             16   /* none known: RWE's RESERVEDLIB_10 */
#define LIB_bb_blitlib              19   /* RWE's list = literal in BB_BlitLib4 */
#define LIB_rishapezlib             21   /* RWE's list */
#define LIB_riseriallib             22   /* literal in Serial-Library (RWE's list: #riblitlib, no source) */
#define LIB_richunkylib             24   /* RWE's list */
#define LIB_ridisplaylib            29   /* RWE's list */
#define LIB_blitzbomberslibtwo      31   /* none known: RWE's RESERVEDLIB_8, next to #blitzbomberslib 30 */
#define LIB_ribbmaplib              34   /* RWE's list */
#define LIB_rithreadlib             44   /* RWE's list */
#define LIB_risnowlib              195   /* literal in SnowLib3 (Acid's #systemcallslib, not in this set) */
/* riarexxlib: the source defines its number itself (#myconst=133 in
   NEWAREXXLIB.bb, !libheader {#myconst,...}); BSS2 RIAREXXLIB has 133 */
#define LIB_riarexxlib             133
#endif
