# Review 09: src/tokens2.c vs blitz2.s 13327-14655

## Findings

- 14389(asm) dolet2: string path returned `*type` = 7 (+flags), but the asm does
  `move varmode,d2` before `tokejsr` (which saves/restores d2), so on return d2 =
  varmode (0 global / $ffff local). dofor's `cmp.b #6,d2` then lets `For a$=..`
  through for a global a$ in the asm, and the C raised badforerr. -> FIXED (quirk
  kept and marked BUG(orig)). Severity: med (edge case, different diagnostics or code).
- 13470/13909(asm) funcret, doend3 (End Function): `thisproc->rettype` is read with
  thisproc == NULL outside a procedure. The asm reads byte 5(0) the same way, and
  endstate/eos then raise badpenderr, so the C is equivalent on the target. It is
  only C UB. -> REPORTED (not changed, behaviour matches the asm on the Amiga).
  Severity: low.
- 13519-13522(asm) dofunc2: the placeholder long is `$4ef9` with d1's stale high word
  (for Statement $ffff4ef9). The C pokes 0x00004ef9. It is always patched in
  endstate unless nomemleft. -> REPORTED (covered by the file's DIFF "garbage
  longs" note). Severity: low.

BUG(orig) in dimwit (`move extraword,(a0)` into varoff/locvaroff on redim): this is
true. The write happens only on the redim path, and dodim2 restores both variables,
so it is harmless.

## Checked and correct

Templates (ongotocode, ongosubcode, ongcode, goset, forset, thebra, thebgt, add1a2,
setles) match the listing byte for byte, and the *f end macros and patch offsets match too
(10 = #labels, d2+2 = bra disp = labcnt*4+2, gc skip offsets 26/26/30).

dothen, ifdone, doongo, dogosub, dogoto, dogoto2, eos_, stateret, funcret,
doreturn, normret, dostate, dofunc, dofunc2 (param opcodes 2940/3940/1940|reg,
stbuff layout, string loop order, nparams nibble), linkput, procfixer (6/10/16
byte layout, bit15 newmem, d4 push for >=5 params), domwait, donewtype,
typemode (alignment, forward pointer types, [n] mulu, flags), doendif3,
doendif2, dowend, dowend2, mydoendif2, mydoendif, doendif, freetheifz,
doend2, doend, doend3 (NewType/If/While/Statement/Function/SetInt/Select/
SetErr), dostop, docont, nop_, freefors, endstate (pend patch, memlib flag,
linksize &~3, string-length fixups fix1/fixq/fixstack with bhi 8), droplocals,
donext (name compare, len-18), donext2 (b/w/l/f selection, both bra
displacements, unsigned range checks), dofor (type check, leaamp, push
sizes, default step 1/quick/ffp, compare selection), dolet, dolet2
(non-string path, .code byte/word/long, bit7 offset word, .nextstr comma
chain, pointer/struct checks), alignletpc, dodim, dodim2 (list, redim
varoff/locvaroff +-4, flags bit8), dimwit (element size, putarr, dims check,
doarr, newarr +1/+3, setles).

Struct offsets (proc, fornode, var, type, ifnode, addrnode, alllocal, blitzlib
flags@12) and callee contracts (find*/addhere* reget, fetchvar, calcvar,
evalconst2 + numd6, tokejsr preserves d2/a3, addoff preserves d2) were checked
against structs.h, parse.h, eval.h, libcall.h and stmts.h.
