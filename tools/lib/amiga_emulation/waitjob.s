; waitjob.s - SNAP:waitjob of the snapshot mode of amiga_emulation
; (amiga_emulation.c, amiga-emulation/README.md "Snapshot mode").
;
; Returns as soon as SNAP:job.script exists: polls it with Lock every
; 2 ticks (40 ms).  The prologue TEST:run.script runs it after it has
; written SNAP:ready; the library saves the state while the waiter
; loops, and every restored session continues in this loop.
;
; 68000 code and dos.library calls of V0 only (OpenLibrary version 0):
; it runs under Kickstart 1.3 (machines a500, a500-68010) and 3.2.
; Assembled in the build by vasm (-Fhunkexe), embedded into the library
; with #embed (tools/lib/amiga_emulation/CMakeLists.txt).

_LVOOpenLibrary		equ	-552
_LVOCloseLibrary	equ	-414
_LVOLock		equ	-84
_LVOUnLock		equ	-90
_LVODelay		equ	-198
ACCESS_READ		equ	-2

	section	code,code

start:	movem.l	d2/a6,-(sp)
	move.l	4.w,a6
	lea	dosname(pc),a1
	moveq	#0,d0
	jsr	_LVOOpenLibrary(a6)
	move.l	d0,a6
loop:	lea	fname(pc),a0
	move.l	a0,d1
	moveq	#ACCESS_READ,d2
	jsr	_LVOLock(a6)
	tst.l	d0
	bne.s	found
	moveq	#2,d1
	jsr	_LVODelay(a6)
	bra.s	loop
found:	move.l	d0,d1
	jsr	_LVOUnLock(a6)
	move.l	a6,a1
	move.l	4.w,a6
	jsr	_LVOCloseLibrary(a6)
	moveq	#0,d0
	movem.l	(sp)+,d2/a6
	rts

dosname:	dc.b	"dos.library",0
fname:		dc.b	"SNAP:job.script",0
	even
