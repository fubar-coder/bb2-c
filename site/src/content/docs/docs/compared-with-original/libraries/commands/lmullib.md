---
title: lmullib
description: The bugs of Blitz Basic 2 in the command library lmullib that the
  BB2 C port fixes.
sidebar:
  order: 37
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/lmullib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Dates and numbers

* A multiplication of longs or quicks whose product does not fit added
  part of the lost high bits back into the result. So `196608*30000` gave
  1603272705 instead of 1603272704, and the quick `300*300` came out
  1/65536 too large; a long was off by at most 65534, a quick by at most
  0.25. The port gives the product with the bits that do not fit cut
  off, and a product that fits is the same as before
  ([#199](https://github.com/fubar-coder/bb2-c/issues/199)); see
  [Getting the old products](#old-products).

#### Getting the old products {#old-products}

A program that lets a long or quick product overflow on purpose and needs
the same numbers as with the original, for example a random number
generator, can compute them with these two functions. Each multiplication
inside them has factors below 65536, which the original and the port both
compute exactly, so they give the original's products with either
library. A call is much slower than the `*` operator.

```
; a*b as the original gave it, for longs
Function.l OldMul{a.l,b.l}
  n.l=0
  If a<0 Then a=-a : n=1-n
  If b<0 Then b=-b : n=1-n
  ah.l=(a LSR 16) AND $FFFF : al.l=a AND $FFFF
  bh.l=(b LSR 16) AND $FFFF : bl.l=b AND $FFFF
  c1.l=ah*bl : c2.l=al*bh
  p.l=al*bl+(((c1+c2) AND $FFFF) LSL 16)+((c1 LSR 16) AND $FFFF)+((c2 LSR 16) AND $FFFF)
  If n Then p=-p
  Function Return p
End Function

; the same for quicks
Function.q OldQMul{a.q,b.q}
  ra.l=Peek.l(&a) : rb.l=Peek.l(&b) : n.l=0
  If ra<0 Then ra=-ra : n=1-n
  If rb<0 Then rb=-rb : n=1-n
  ah.l=(ra LSR 16) AND $FFFF : al.l=ra AND $FFFF
  bh.l=(rb LSR 16) AND $FFFF : bl.l=rb AND $FFFF
  hh.l=ah*bh
  p.l=(((al*bl) LSR 16) AND $FFFF)+ah*bl+al*bh+(hh LSL 16)+((hh LSR 16) AND $FFFF)
  If n Then p=-p
  r.q=0 : Poke.l &r,p
  Function Return r
End Function

NPrint OldMul{196608,30000}   ; 1603272705, as with the original
```

::fixed-bugs-library{part="elsewhere"}
