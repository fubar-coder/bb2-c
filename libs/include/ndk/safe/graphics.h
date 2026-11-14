/* The safe forms of <inline/graphics.h> for the libraries
   (cmake/ndk-calls.cmake, libs/include/ndk/functions.list): the NDK's
   macro, the register variable on a3 replaced by an operand in a register
   gcc chooses, which "exg" swaps with a3 around the jsr.  a3 is back
   afterwards (the OS keeps d2-d7/a2-a6); 4 bytes per call, no stack, the
   condition codes untouched (the NDK's own technique for a5). */

#define __TextFit_base(__in_base, ___rp, ___string, ___strLen, ___textExtent, ___constrainingExtent, ___strDirection, ___constrainingBitWidth, ___constrainingBitHeight) ({\
  struct RastPort * __p____rp = (struct RastPort *)(___rp);\
  STRPTR __p____string = (STRPTR)(___string);\
  UWORD __p____strLen = (UWORD)(___strLen);\
  struct TextExtent * __p____textExtent = (struct TextExtent *)(___textExtent);\
  struct TextExtent * __p____constrainingExtent = (struct TextExtent *)(___constrainingExtent);\
  WORD __p____strDirection = (WORD)(___strDirection);\
  UWORD __p____constrainingBitWidth = (UWORD)(___constrainingBitWidth);\
  UWORD __p____constrainingBitHeight = (UWORD)(___constrainingBitHeight);\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register ULONG __v_ret __asm("d0");\
  register struct RastPort * __v0 __asm("a1") = __p____rp;\
  register STRPTR __v1 __asm("a0") = __p____string;\
  register ULONG __v2 __asm("d0") = __p____strLen;\
  register struct TextExtent * __v3 __asm("a2") = __p____textExtent;\
  register LONG __v5 __asm("d1") = __p____strDirection;\
  register ULONG __v6 __asm("d2") = __p____constrainingBitWidth;\
  register ULONG __v7 __asm("d3") = __p____constrainingBitHeight;\
  __asm volatile (\
                   "exg %4,%/a3\n\tjsr %/a6@(-696:W)\n\texg %4,%/a3\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1), "+d"(__v5), "+r"(__p____constrainingExtent)\
                   : "a"(__p__in_base), "d"(__v2), "a"(__v3), "d"(__v6), "d"(__v7)\
                   : "fp0", "fp1", "cc", "memory" );\
  __v_ret;})

#define __ChangeExtSpriteA_base(__in_base, ___vp, ___oldsprite, ___newsprite, ___tags) ({\
  struct ViewPort * __p____vp = (struct ViewPort *)(___vp);\
  struct ExtSprite * __p____oldsprite = (struct ExtSprite *)(___oldsprite);\
  struct ExtSprite * __p____newsprite = (struct ExtSprite *)(___newsprite);\
  struct TagItem * __p____tags = (struct TagItem *)(___tags);\
  register void * __p__in_base __asm("a6") = (void *)(__in_base);\
  register LONG __v_ret __asm("d0");\
  register struct ViewPort * __v0 __asm("a0") = __p____vp;\
  register struct ExtSprite * __v1 __asm("a1") = __p____oldsprite;\
  register struct ExtSprite * __v2 __asm("a2") = __p____newsprite;\
  __asm volatile (\
                   "exg %3,%/a3\n\tjsr %/a6@(-1026:W)\n\texg %3,%/a3\n"\
                   : "=d"(__v_ret), "+a"(__v0), "+a"(__v1), "+r"(__p____tags)\
                   : "a"(__p__in_base), "a"(__v2)\
                   : "fp0", "fp1", "cc", "memory", "d1" );\
  __v_ret;})
