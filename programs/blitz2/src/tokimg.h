/*
 * tokimg.h - what a token image (one object holding the nodes of
 * toklist.h, main.c's tokimg and ab3tok.c's AmiBlitz3 list) is built of:
 * the asmmac equates its asm data uses, the struct member of a node and
 * the pointer to a node of the image IMG.  The includer defines TK, TA,
 * TKLAST, TK3, TK3LAST, M and IMG (toklist.h).
 */
#ifndef TOKIMG_H
#define TOKIMG_H

/* asmmac equates (EA masks / size codes).  Renamed with a trailing `_`
   because `long`, `word`, `byte` ... clash with C / NDK names. */
#define ndn_    0xfffe
#define nan_    0xfffd
#define nian_   0xfffb
#define nanp_   0xfff7
#define npan_   0xffef
#define ndan_   0xffdf
#define ndanxi_ 0xffbf
#define nabsw_  0xff7f
#define nabsl_  0xfeff
#define ndpc_   0xfdff
#define ndpcxi_ 0xfbff
#define nimm_   0xf7ff
#define nccr_   0xefff
#define nsr_    0xdfff
#define nusp_   0xbfff
#define nmem_   0x7fff
#define nall_   0
#define dn_     1
#define an_     2
#define ian_    4
#define anp_    8
#define pan_    16
#define dan_    32
#define danxi_  64
#define absw_   128
#define absl_   256
#define dpc_    512
#define dpcxi_  1024
#define imm_    2048
#define ccr_    4096
#define sr_     8192
#define usp_    16384
#define mem_    32768
#define all_    0xfff
#define byte_   0xff0
#define word_   0xf1f
#define long_   0x2ff
#define sall_   0x210
#define nosz_   0xfff
#define byteword_ 0xf10
#define wordlong_ 0x10f

/* the struct member of a node: next.l num.w name\0 help\0 (even); of an
   asm mnemonic node, its struct asment at the next even address */
#define TKMEMBER(id, name, help) \
    struct { struct token *next; UWORD num; char s[sizeof(name "\0" help)]; } id;
#define TAMEMBER(id, name) \
    struct { struct token *next; UWORD num; char s[sizeof(name "\0" "")]; struct asment d; } id;
/* the node id of the image IMG, as a next pointer */
#define NX(id) (struct token *)&IMG.id

#endif
