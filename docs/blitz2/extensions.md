# Extensions of blitz2

`blitz2` compiles a Blitz Basic 2 source to the same executable as the
CD's compiler. Its extensions only give a meaning to sources the original
refuses.

## `#__debug`

`#__debug` is a constant the compiler defines itself (AmiBlitz3's name and
values; in `blitz2-ab3` in both modes). Its value is −1 when the program
is compiled with runtime error checking and 0 without it. The option is
the one given at the start of the compile; `RunErrsOff` in the source does
not change it. The original compiler says "Constant Not Found" for it.

Use it to keep code for debugging only in a program that is compiled with
runtime error checking:

```
CNIF #__debug
  NPrint "only with runtime error checking"
CEND
```

Details:

* It can be used wherever a constant can: in expressions, in `CNIF`, in
  constant definitions and as an assembler operand.
* A source's own `#__debug=…` replaces the predefined one, as in
  AmiBlitz3.
* A resident's `#__debug` (AmiBlitz3's `all.res` has one) gets the value
  of the compile.
* `Stop` itself stays as in Blitz Basic 2: without runtime error checking
  it reports "Debugger must be enabled for STOP". Wrap it in
  `CNIF #__debug … CEND` for a program that is also compiled without.

## Tests

* `blitz2_debug_on`, `blitz2_debug_off`, `blitz2_stop_off`
  (`tests/blitz2/`; the section "emulator tests" of `tests/README.md`
  describes how they run);
* in AB3 mode `debugconst_on`, `debugconst_off`, `debugconst_r`,
  `debugconst_res`, `debugconst_res_r` of `ab3_cli`;
* `ab3_cli_ref` confirms `debugconst_off` (also the replacing definition)
  with AmiBlitz3, whose command line has no debugger, so `debugconst_on`
  and `debugconst_res` give 0 there (`docs/ab3/variant.md`).
