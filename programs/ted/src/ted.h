/*
 * ted.h - umbrella of the Amiga side: core headers + structs.h + glue.h +
 * every chunk header.  Each src/NAME.c includes this (the core files include
 * only their core headers).  Owner: coordinator.
 */
#ifndef TED_TED_H
#define TED_TED_H

#include "tedcore.h"
#include "hooks.h"
#include "state.h"
#include "loadsave.h"
#include "lines.h"
#include "services.h"
#include "fileio.h"
#include "tokit.h"
#include "lineedit.h"
#include "keymap.h"

#include "structs.h"
#include "glue.h"
#include "main.h"
#include "edit.h"
#include "mouse.h"
#include "reqs.h"
#include "screen.h"
#include "input.h"
#include "data.h"
#include "rawkey.h"
#include "filereq.h"

#endif
