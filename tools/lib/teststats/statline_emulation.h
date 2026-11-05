/*
 * statline_emulation.h - the @stat line (statline.h) of an emulator
 * session (amiga_emulation.h), for the test runners: fill it after
 * amiga_emulation_join and before amiga_emulation_free, set the verdict
 * once it is known, write it with statline_write.
 */
#ifndef STATLINE_EMULATION_H
#define STATLINE_EMULATION_H

#include <string.h>
#include "amiga_emulation.h"
#include "statline.h"

/**
 * @brief Fill the @stat line of an emulator session.
 *
 * The verdict is not set here: the caller sets it once it is known.
 *
 * @param s       out: the line (initialised here, statline_init)
 * @param work    the work directory of the pass: its last part is the test
 * @param config  the configuration
 * @param e       the session (after amiga_emulation_join)
 */
static inline void statline_session(struct statline *s, const char *work, const char *config,
                                    const struct amiga_emulation *e)
{
    static const char *const ends[] = { [AMIGA_EMULATION_DONE] = "done",
                                        [AMIGA_EMULATION_TIMEOUT] = "timeout",
                                        [AMIGA_EMULATION_HALT] = "halt",
                                        [AMIGA_EMULATION_EXITED] = "exited",
                                        [AMIGA_EMULATION_FAILED] = "failed" };
    struct amiga_emulation_stats st;
    const char *base = work ? strrchr(work, '/') : NULL;

    amiga_emulation_stats(e, &st);
    statline_init(s);
    statline_set(s->test, sizeof s->test, base && base[1] ? base + 1 : work);
    statline_set(s->config, sizeof s->config, config);
    statline_set(s->machine, sizeof s->machine, st.machine);
    statline_set(s->snapshot, sizeof s->snapshot, amiga_emulation_snapshot_use_name(st.snapshot));
    statline_set(s->end, sizeof s->end,
                 (unsigned)st.status < sizeof ends / sizeof ends[0] ? ends[st.status] : "failed");
    s->session = st.session;
    s->wait = st.wait;
    s->timed = st.timed;
    s->limit = st.timeout;
}

#endif
