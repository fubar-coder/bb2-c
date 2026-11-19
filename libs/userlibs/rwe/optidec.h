/*
 * optidec.h - the decruncher of StoneCracker 4.04 ("S404"), optidec.bin
 * (third party code that the RWE sources stclib.bb and stcdecrunchlib.bb
 * include with IncBin; its source is optidec.S v1.1 of StoneCracker 4.10,
 * Aminet util/pack/Stc4103.lha), as assembler text for
 * ristonecrackerlib.c and ristcdecrunchlib.c (each puts OPTIDEC_ASM in
 * a BLITZ_ASM_BLOCK of its own).
 *
 * Assembler for speed (perf bstc: in C the decruncher took 1.8 times the
 * original's cycles): the loop is optidec.S instruction by instruction,
 * with the changes below; the unpacked data are the original's for every
 * input it handles (see DIFF).
 *
 *   sd_dec   In:  a0 the destination, a1 the S404 file (its header)
 *            Out: d0 = 1 and a0 = the destination for data unpacked,
 *                 d0 = 0 for refused (corrupt) data
 *            Keeps a2-a6 (MOVEM.l on the stack), changes d1-d7/a0-a1.
 *
 * The registers are the original's: d0 = 1 (the 1 of its ADDX), d2 the
 * far offset bits, d3 the count (its upper word is kept 0: MOVEQ #-1,d3
 * became a word -1), d4 = $FFFF, d5 = 16, d6 the bit buffer (bits at the
 * top of its low word), d7 the bits left in it, a1 the packed words (read
 * backwards), a0 the output (written backwards), a2 a match, a3/a4 the
 * main loop's return addresses of the bit reader (P_bits, back by JMP
 * (a6)), a5 the loop's limit.
 *
 * Faster than the original: the 101xx match (4..7 bytes, the most
 *   frequent code in text) reads its 2 bits one at a time (P_m47, as the
 *   original reads the 11x match's bit) instead of through the bit reader:
 *   with d7 = 0..15, as in the main loop, the bits, d6's low word and d7
 *   come out the same (d6's upper word differs; every later use of it
 *   clears or shifts it out first).
 * Fixed (#833): corrupt data no longer write below the destination; the
 *   original wrote a match or literal run that starts just above it past
 *   its start, into the memory below.  The checks cost nothing per token
 *   on the main path: the loop's one test (CMP.l a0,a5) compares with
 *   dest + SD_LIMIT instead of dest, so every token but a long match of
 *   more than one length byte ends at or above dest, and such a match is
 *   checked (sm_more); the last bytes are unpacked by a second copy of the
 *   loop (labels st_*) that checks each literal run and match against the
 *   room left.  Once per call: far offset bits (the efficiency) above 16
 *   and an odd packed length are refused.  Corrupt data now give 0 and
 *   write only inside dest .. dest + unpacked length.
 *   Not checked (a known limit): reads.  A stream that runs out reads
 *   words below the packed data (the header, then the memory below the
 *   file), and a match may read up to 544 + 65535 bytes above the output
 *   written so far, also above dest + unpacked length; such data unpack
 *   wrong, but nothing is written outside the destination.
 * DIFF: the literal run reads its shift count from a table indexed by the
 *   bit count d7 (MOVE.b table(pc,d7.w)); d7 leaves 0..15 only with a
 *   first bit count (the trailer's third word) above 16, and then the
 *   original reads whatever lies around the table (its own code) - this
 *   build takes the index modulo 16.  With d7 = 0..15 the result is the
 *   same.  A file with a first bit count above 16 is unpacked by the tail
 *   loop alone, which takes the index modulo 16; the main loop needs no
 *   AND.
 */
#ifndef OPTIDEC_H
#define OPTIDEC_H

/* the main loop's limit, dest + SD_LIMIT: the longest output of one token
   but a long match of more than one length byte (23 + 254 bytes; a
   literal byte 1, the matches 2..3, 4..7, 8..22, a literal run 14..45),
   less 1 */
#define SD_LIMIT "276"

/*
 * SD_LOOP: the decruncher's loop (optidec.S from _lins on), twice: the
 * main loop (P "sm") and the tail (P "st").
 *   exit       the code at P_exit, where the loop ends (a0 <= a5)
 *   loop_a6    the length's return address into a6 (P_len5a)
 *   off5_a6    the offset's return address into a6 (P_off6)
 *   more       where a long match's length byte 255 goes on (a label)
 *   more_code  code behind the loop (the main loop: its continuation
 *              and its exit)
 *   chk_match  after a match's offset, before its bytes (fixed #833)
 *   run_shift  a literal run's shift count from P_tab into d1 (MOVE.b
 *              table(pc,d7.w),d1; the tail checks the run first)
 */
#define SD_LOOP(P, S, exit, loop_a6, off5_a6, more, more_code, chk_match,  \
        run_shift)                                                          \
P "_m47:\n"                             /* 101xx: the 2 bits one by one */  \
"   dbf     d7," P "_m47a\n"                                                \
"   move.w  -(a1),d6\n"                                                     \
"   moveq   #15,d7\n"                                                       \
P "_m47a:\n"                                                                \
"   add.w   d6,d6\n"                                                        \
"   addx.w  d3,d3\n"                    /* d3 = 2: 4 + bit */               \
"   dbf     d7," P "_m47b\n"                                                \
"   move.w  -(a1),d6\n"                                                     \
"   moveq   #15,d7\n"                                                       \
P "_m47b:\n"                                                                \
"   add.w   d6,d6\n"                                                        \
"   addx.w  d3,d3\n"                    /* 8 + the 2 bits */                \
"   subq.w  #6,d3\n"                    /* 2 + the 2 bits */                \
"   dbf     d7," P "_off2\n"                                                \
"   bra     " P "_off2a\n"                                                  \
P "_ins:\n"                             /* _lins: 8 bits, a literal byte */ \
"   subq.w  #8,d7\n"                                                        \
"   bpl.s   " P "_ins2\n"                                                   \
"   move.w  d7,d1\n"                    /* across the next word */          \
"   addq.w  #8,d7\n"                                                        \
"   lsl.l   d7,d6\n"                                                        \
"   move.w  -(a1),d6\n"                                                     \
"   neg.w   d1\n"                                                           \
"   lsl.l   d1,d6\n"                                                        \
"   addq.w  #8,d7\n"                                                        \
"   swap    d6\n"                                                           \
"   move.b  d6,-(a0)\n"                                                     \
"   swap    d6\n"                                                           \
"   cmp.l   a0,a5\n"                                                        \
"   dbhs    d7," P "_main\n"                                                \
"   bra.s   " P "_exma\n"                                                   \
P "_ins2:\n"                                                                \
"   rol.w   #8,d6\n"                                                        \
"   move.b  d6,-(a0)\n"                                                     \
P "_test1:\n"                           /* the loop's test */               \
"   cmp.l   a0,a5\n"                                                        \
"   dbhs    d7," P "_main\n"                                                \
P "_exma:\n"                                                                \
"   bhs" S "   " P "_exit\n"                                                \
P "_main1:\n"                                                               \
"   move.w  -(a1),d6\n"                                                     \
"   moveq   #15,d7\n"                                                       \
P "_main:\n"                                                                \
"   add.w   d6,d6\n"                                                        \
"   bcc.s   " P "_ins\n"                                                    \
"   dbf     d7," P "_len1\n"                                                \
"   move.w  -(a1),d6\n"                                                     \
"   moveq   #15,d7\n"                                                       \
P "_len1:\n"                                                                \
"   add.w   d6,d6\n"                                                        \
"   bcs.s   " P "_len6\n"               /* 11x: 2 or 3 bytes */             \
"   dbf     d7," P "_len2\n"                                                \
"   move.w  -(a1),d6\n"                                                     \
"   moveq   #15,d7\n"                                                       \
P "_len2:\n"                                                                \
"   moveq   #2,d1\n"                                                        \
"   moveq   #2,d3\n"                                                        \
"   add.w   d6,d6\n"                                                        \
"   bcs.s   " P "_m47\n"                /* 101xx: 4..7 bytes */             \
"   dbf     d7," P "_len3\n"                                                \
"   move.w  -(a1),d6\n"                                                     \
"   moveq   #15,d7\n"                                                       \
P "_len3:\n"                                                                \
"   add.w   d6,d6\n"                                                        \
"   bcc.s   " P "_len4\n"               /* 1000: a long match */            \
"   moveq   #4,d1\n"                    /* 1001xxxx: 8..22 bytes */         \
"   moveq   #6,d3\n"                                                        \
"   lea     " P "_len3a(pc),a6\n"                                           \
"   bra" S "   " P "_bits\n"                                                \
P "_len3a:\n"                                                               \
"   add.w   d1,d3\n"                                                        \
"   cmp.w   #15,d1\n"                                                       \
"   blo.s   " P "_off1\n"                                                   \
"   moveq   #5,d1\n"                    /* 10011111xxxxx: 14..45 literals */\
"   moveq   #14,d3\n"                                                       \
"   lea     " P "_len3b(pc),a6\n"                                           \
"   bra" S "   " P "_bits\n"                                                \
P "_len4:\n"                                                                \
"   moveq   #21,d3\n"                                                       \
P "_loop:\n"                            /* 23 + n bytes, n in bytes */      \
"   moveq   #8,d1\n"                                                        \
P "_len5:\n"                                                                \
loop_a6                                                                     \
"   bra" S "   " P "_bits\n"                                                \
P "_len5a:\n"                           /* the long match: + 8 bits */      \
"   add.w   d1,d3\n"                                                        \
"   not.b   d1\n"                                                           \
"   dbeq    d7," P "_off2\n"                                                \
"   bne.s   " P "_off2a\n"                                                  \
"   bra     " more "\n"                 /* 255: one byte more */            \
P "_off6:\n"                            /* the match: d3 + 2 bytes */       \
"   add.w   d1,a2\n"                                                        \
chk_match                                                                   \
"   move.b  (a2),-(a0)\n"                                                   \
P "_copy:\n"                                                                \
"   move.b  -(a2),-(a0)\n"                                                  \
"   dbf     d3," P "_copy\n"                                                \
P "_test:\n"                                                                \
"   cmp.l   a0,a5\n"                                                        \
"   dbhs    d7," P "_main\n"                                                \
"   blo" S "   " P "_main1\n"                                               \
P "_exit:\n"                                                                \
exit                                                                        \
P "_len6:\n"                                                                \
"   dbf     d7," P "_len7\n"                                                \
"   move.w  -(a1),d6\n"                                                     \
"   moveq   #15,d7\n"                                                       \
P "_len7:\n"                                                                \
"   add.w   d6,d6\n"                                                        \
"   addx.w  d0,d3\n"                                                        \
P "_off1:\n"                                                                \
"   dbf     d7," P "_off2\n"                                                \
P "_off2a:\n"                                                               \
"   move.w  -(a1),d6\n"                                                     \
"   moveq   #15,d7\n"                                                       \
P "_off2:\n"                            /* the offset */                    \
"   add.w   d6,d6\n"                                                        \
"   bcs.s   " P "_off3\n"                                                   \
"   dbf     d7," P "_off4\n"                                                \
"   move.w  -(a1),d6\n"                                                     \
"   moveq   #15,d7\n"                                                       \
P "_off4:\n"                                                                \
"   moveq   #9,d1\n"                    /* 00: 9 bits + 32 */               \
"   lea     32(a0),a2\n"                                                    \
"   add.w   d6,d6\n"                                                        \
"   bcc.s   " P "_off5\n"                                                   \
"   moveq   #5,d1\n"                    /* 01: 5 bits */                    \
"   move.l  a0,a2\n"                                                        \
"   bra.s   " P "_off5\n"                                                   \
P "_off3:\n"                                                                \
"   lea     544(a0),a2\n"               /* 1: d2 bits + 544 */              \
"   move.w  d2,d1\n"                                                        \
P "_off5:\n"                                                                \
off5_a6                                                                     \
P "_bits:\n"                            /* d1 bits -> d1, back by a6 */     \
"   and.l   d4,d6\n"                                                        \
"   sub.w   d1,d7\n"                                                        \
"   bpl.s   " P "_bits2\n"                                                  \
"   add.w   d7,d1\n"                                                        \
"   lsl.l   d1,d6\n"                                                        \
"   move.w  d7,d1\n"                                                        \
"   move.w  -(a1),d6\n"                                                     \
"   neg.w   d1\n"                                                           \
"   add.w   d5,d7\n"                                                        \
P "_bits2:\n"                                                               \
"   lsl.l   d1,d6\n"                                                        \
"   move.l  d6,d1\n"                                                        \
"   swap    d1\n"                                                           \
"   jmp     (a6)\n"                                                         \
P "_pins2:\n"                           /* d3 = 0: MOVEQ #-1,d3, the */     \
"   subq.w  #1,d3\n"                    /* upper word stays 0 */            \
"   bra     " P "_ins2\n"                                                   \
P "_2ins2:\n"                           /* the literal run, 2 bytes a time */\
"   rol.w   #8,d6\n"                                                        \
"   move.b  d6,-(a0)\n"                                                     \
P "_2ins1:\n"                                                               \
"   lsl.l   d7,d6\n"                                                        \
"   move.w  -(a1),d6\n"                                                     \
"   lsl.l   d1,d6\n"                                                        \
"   swap    d6\n"                                                           \
"   move.b  d6,-(a0)\n"                                                     \
"   swap    d6\n"                                                           \
"   subq.w  #2,d3\n"                                                        \
"   bgt.s   " P "_2ins2\n"                                                  \
"   beq.s   " P "_pins2\n"                                                  \
"   addq.w  #8,d7\n"                                                        \
"   bra" S "   " P "_test\n"                                                \
P "_len3b:\n"                           /* the literal run: d3 bytes */     \
"   add.w   d1,d3\n"                                                        \
run_shift                                                                   \
"   bpl.s   " P "_2ins1\n"                                                  \
"   subq.w  #8,d7\n"                                                        \
"   dbf     d3," P "_2ins2\n"           /* d3 = 14..45: always taken */     \
P "_tab:\n"     /* the shift count of the run's straddling byte (_lnewd1) */\
"   .byte   0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01\n"                      \
"   .byte   0x88,0x87,0x86,0x85,0x84,0x83,0x82,0x81\n"                      \
more_code

/* sd_dec (see the head of the file) */
#define OPTIDEC_ASM                                                         \
"sd_dec:\n"                                                                 \
"   movem.l a2/a5-a6,-(sp)\n"           /* a3-a4: for the main loop */     \
"   addq.w  #8,a1\n"                    /* skip "S404" and the offset */    \
"   move.l  a0,a5\n"                                                        \
"   add.l   (a1)+,a0\n"                 /* + the unpacked length */         \
"   add.l   (a1),a1\n"                  /* + the packed length */           \
"   moveq   #1,d0\n"                                                        \
"   move.w  a1,d1\n"                    /* fixed (#833): the trailer is */  \
"   and.w   d0,d1\n"                    /* words (the file is even) */      \
"   bne     sd_fail\n"                                                      \
"   movem.w (a1),d2/d6/d7\n"            /* far bits, bit buffer, bits */    \
"   moveq   #16,d5\n"                                                       \
"   cmp.w   d5,d2\n"                    /* fixed (#833): far bits 0..16 */  \
"   bhi     sd_fail\n"                                                      \
"   moveq   #0,d4\n"                                                        \
"   not.w   d4\n"                                                           \
"   move.l  d4,d3\n"                    /* MOVEQ #-1,d3; the upper word 0 */\
"   lea     " SD_LIMIT "(a5),a2\n"      /* fixed (#833): the limit */       \
"   cmp.l   a0,a2\n"                                                        \
"   bhs     st_test1\n"                 /* all in the tail */               \
"   cmp.w   d5,d7\n"                    /* DIFF: a first bit count above */ \
"   bhi     st_test1\n"                 /* 16: the tail, P_tab index & 15 */\
"   move.l  a2,a5\n"                                                        \
"   movem.l a3-a4,-(sp)\n"                                                  \
"   lea     sm_off6(pc),a3\n"                                               \
"   lea     sm_len5a(pc),a4\n"                                              \
"   bra     sm_test1\n"                                                     \
SD_LOOP("sm", ".s",                                                               \
        "   bra     sm_exit2\n",                                            \
        "   move.l  a4,a6\n",                                               \
        "   move.l  a3,a6\n",                                               \
        "sm_more",                                                          \
        /* the rest in the tail, the limit dest */                          \
        "sm_exit2:\n"                                                       \
        "   movem.l (sp)+,a3-a4\n"                                          \
        "   lea     -" SD_LIMIT "(a5),a5\n"                                 \
        "   bra     st_test1\n"                                             \
        /* a long match of more than one length byte (more than 277 */      \
        /* bytes): its bytes are checked (fixed #833) */                    \
        "sm_more:\n"                                                        \
        "   moveq   #8,d1\n"                                                \
        "   lea     sm_len5c(pc),a6\n"                                      \
        "   bra     sm_bits\n"                                              \
        "sm_len5c:\n"                                                       \
        "   add.w   d1,d3\n"                                                \
        "   not.b   d1\n"                                                   \
        "   beq.s   sm_more\n"                                              \
        /* dest + d3 + 2 <= a0, dest = a5 - SD_LIMIT */                     \
        "   lea     -" SD_LIMIT "+2(a5),a2\n"                               \
        "   add.l   d3,a2\n"                                                \
        "   cmp.l   a0,a2\n"                                                \
        "   bhi.s   sm_fail\n"                                              \
        "   dbf     d7,sm_off2\n"                                           \
        "   bra     sm_off2a\n",                                            \
        "",                                                                 \
        "   move.b  sm_tab(pc,d7.w),d1\n")                                  \
"sm_fail:\n"                                                                \
"   movem.l (sp)+,a3-a4\n"                                                  \
"   bra     sd_fail\n"                                                      \
SD_LOOP("st", "",                                                               \
        "sd_ok:\n"                      /* d0 = 1 */                        \
        "   movem.l (sp)+,a2/a5-a6\n"                                       \
        "   rts\n",                                                         \
        "   lea     st_len5a(pc),a6\n",                                     \
        "   lea     st_off6(pc),a6\n",                                      \
        "st_loop",                                                          \
        "",                                                                 \
        /* fixed (#833): a5 + d3 + 2 <= a0 */                               \
        "   lea     2(a5,d3.l),a6\n"                                        \
        "   cmp.l   a0,a6\n"                                                \
        "   bhi     sd_fail\n",                                             \
        /* fixed (#833): a5 + d3 <= a0 (d3 = 14..45) */                     \
        "   lea     0(a5,d3.l),a6\n"                                        \
        "   cmp.l   a0,a6\n"                                                \
        "   bhi     sd_fail\n"                                              \
        "   moveq   #15,d1\n"                  /* DIFF: the index & 15 */  \
        "   and.w   d7,d1\n"                                                \
        "   move.b  st_tab(pc,d1.w),d1\n")                                  \
"sd_fail:\n"                                                                \
"   moveq   #0,d0\n"                                                        \
"   movem.l (sp)+,a2/a5-a6\n"                                               \
"   rts\n"

#endif
