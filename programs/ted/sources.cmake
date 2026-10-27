# The modules of TED (used by programs/ted and its host tests).
# Core modules: no AmigaOS calls (they also build natively on a host, for
# unit tests); everything else is the AmigaOS part.
set(TED_CORE_MODULES state loadsave lines services fileio tokit lineedit keymap)
set(TED_AMIGA_MODULES main edit mouse reqs screen input data filereq rawkey)
