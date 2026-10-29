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
#endif
