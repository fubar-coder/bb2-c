# Tasks

## 1. Codec

- [x] 1.1 The writer and the reader of result entries in `shared/basictest/` (every entry kind of the spec, the encodings, the format version; the writer without allocation and division, into a caller's buffer); verify: host tests (snitch, one program per test file) of a round trip of every entry kind and of each refused input (unknown entry, wrong field count, malformed value, unknown version) naming the line, with and without `-DHOST_SANITIZE=ON`
- [x] 1.2 A build check that compiles the writer in the library environment of `cmake/BlitzLibrary.cmake`, with a test with deliberately broken input (a writer that calls a division helper); verify: the check fails on the broken input and passes on the codec; a clean rebuild has no warnings

## 2. Macro text and documentation

- [ ] 2.1 The macro text `shared/basictest/macros.bas` as in the spec (blocks with the `Dc.l` end reference, fixed labels of init and end, check macros with `K` first); verify: a host test that the text defines every macro of the spec with the argument order of the spec
- [ ] 2.2 The format, the source form and the preprocessed form in `tests/README.md`; verify: the examples of the text are read back by the codec in a host test

## Workflow follow-up

- Archive this change before `basic-testlib` and `basic-test-runner` start their implementation.
