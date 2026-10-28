/*
 * serial.c - the remote end of the serial tests, see serial.h.
 */
#define _XOPEN_SOURCE 700       /* strdup */
#define _DEFAULT_SOURCE         /* usleep */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "serial.h"
#include "util.h"

struct session {
    char *name;
    struct text raw;            /* everything received (bytes, may contain 0) */
    struct text log;            /* transcript */
    struct text rx;             /* received, not yet in the transcript */
    struct text data;           /* data bytes since the previous request */
};

struct responder {
    unsigned port;
    pthread_t thread;
    volatile int stop;
    struct session *s;
    size_t ns;
    struct session *cur;
    unsigned connects;
};

/* bytes into a text (text_add is for strings) */
static void bytes_add(struct text *t, const uint8_t *p, size_t n)
{
    size_t i;

    for (i = 0; i < n && !t->bad; i++) {
        if (t->n + 2 > t->cap) {
            char *q = realloc(t->p, t->cap = (t->n + 2) * 2);

            if (!q) {
                t->bad = 1;
                return;
            }
            t->p = q;
        }
        t->p[t->n++] = (char)p[i];
        t->p[t->n] = 0;
    }
}

/* printable form for the transcript */
static void esc(struct text *t, const struct text *d)
{
    size_t i;

    for (i = 0; i < d->n; i++) {
        uint8_t c = (uint8_t)d->p[i];

        if (c >= 32 && c < 127 && c != '\\')
            text_add(t, "%c", c);
        else
            text_add(t, "\\x%02x", c);
    }
}

static void flush_rx(struct responder *r)
{
    struct session *s = r->cur;

    if (s && s->rx.n) {
        text_add(&s->log, "<< ");
        esc(&s->log, &s->rx);
        text_add(&s->log, "\n");
        s->rx.n = 0;
    }
}

static void begin(struct responder *r, const struct text *name)
{
    struct session *s = realloc(r->s, (r->ns + 1) * sizeof *s);
    static const uint8_t one = 1, two = 2;

    flush_rx(r);
    if (!s) {
        r->cur = NULL;
        return;
    }
    r->s = s;
    s = &r->s[r->ns++];
    memset(s, 0, sizeof *s);
    s->name = strndup(name->p ? name->p : "", name->n);
    bytes_add(&s->raw, &one, 1);        /* the start marker belongs to it */
    bytes_add(&s->raw, (const uint8_t *)name->p, name->n);
    bytes_add(&s->raw, &two, 1);
    text_add(&s->log, "-- start %s\n", s->name ? s->name : "");
    r->cur = s;
}

static void reply(struct responder *r, int sock, uint8_t code)
{
    static const uint8_t d[] = { 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87 };
    struct session *s = r->cur;
    struct text out = { 0 };
    uint8_t l[100];
    int i;

    flush_rx(r);
    switch (code) {
    case 'A': bytes_add(&out, (const uint8_t *)"Z", 1); break;
    case 'B': bytes_add(&out, (const uint8_t *)"HELLO", 5); break;
    case 'C': bytes_add(&out, (const uint8_t *)"0123456789ABCDEF", 16); break;
    case 'D': bytes_add(&out, d, sizeof d); break;
    case 'E': bytes_add(&out, (const uint8_t *)s->data.p, s->data.n); break;
    case 'L':
        for (i = 0; i < 100; i++)
            l[i] = (uint8_t)(0x30 + i % 40);
        bytes_add(&out, l, sizeof l);
        break;
    case 'W':
        usleep(100000);
        bytes_add(&out, (const uint8_t *)"w", 1);
        break;
    default: bytes_add(&out, (const uint8_t *)"?", 1);
    }
    s->data.n = 0;
    text_add(&s->log, ">> ");
    esc(&s->log, &out);
    text_add(&s->log, "\n");
    if (out.n)                          /* fails only if the emulator went away */
        (void)send(sock, out.p, out.n, MSG_NOSIGNAL);
    free(out.p);
}

/* one connection: the protocol of serial.h */
static void serve(struct responder *r, int sock)
{
    enum { IDLE, NAME, CODE } state = IDLE;
    struct text name = { 0 };

    while (!r->stop) {
        struct pollfd pf = { sock, POLLIN, 0 };
        uint8_t buf[4096];
        ssize_t n, i;

        if (poll(&pf, 1, 200) <= 0)
            continue;
        if ((n = recv(sock, buf, sizeof buf, 0)) <= 0)
            break;                      /* Amiberry closed the port */
        for (i = 0; i < n; i++) {
            uint8_t c = buf[i], b = c & 0x7F;
            struct session *s = r->cur;

            if (s) {
                bytes_add(&s->raw, &c, 1);
                bytes_add(&s->rx, &c, 1);
            }
            if (state == NAME) {
                if (b == 2) {
                    begin(r, &name);
                    state = IDLE;
                } else
                    bytes_add(&name, &b, 1);
            } else if (state == CODE) {
                state = IDLE;
                reply(r, sock, b);
            } else if (b == 1) {
                state = NAME;
                name.n = 0;
            } else if (b == 5 && s)
                state = CODE;
            else if (b == 4 && s) {
                flush_rx(r);
                text_add(&s->log, "-- end\n");
                r->cur = NULL;
            } else if (s)
                bytes_add(&s->data, &c, 1);
        }
    }
    free(name.p);
}

static void *run(void *arg)
{
    struct responder *r = arg;

    while (!r->stop) {
        struct sockaddr_in a = { 0 };
        int sock = socket(AF_INET, SOCK_STREAM, 0);

        a.sin_family = AF_INET;
        a.sin_port = htons((uint16_t)r->port);
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (sock < 0 || connect(sock, (struct sockaddr *)&a, sizeof a)) {
            if (sock >= 0)
                close(sock);
            usleep(200000);
            continue;
        }
        r->connects++;
        serve(r, sock);
        close(sock);
    }
    return NULL;
}

unsigned serial_free_port(void)
{
    struct sockaddr_in a = { 0 };
    socklen_t len = sizeof a;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    unsigned port = 0;

    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (sock >= 0 && !bind(sock, (struct sockaddr *)&a, sizeof a)
        && !getsockname(sock, (struct sockaddr *)&a, &len))
        port = ntohs(a.sin_port);
    if (sock >= 0)
        close(sock);
    return port;
}

struct responder *serial_start(unsigned port)
{
    struct responder *r = calloc(1, sizeof *r);

    if (!r)
        return NULL;
    r->port = port;
    if (pthread_create(&r->thread, NULL, run, r)) {
        free(r);
        return NULL;
    }
    return r;
}

int serial_stop(struct responder *r, const char *dir)
{
    size_t i;
    int n = 0, bad = 0;

    r->stop = 1;
    pthread_join(r->thread, NULL);
    r->cur = NULL;
    for (i = 0; i < r->ns; i++) {
        struct session *s = &r->s[i];
        char *raw = str_printf("%s/%s.serial", dir, s->name ? s->name : "unnamed");
        char *txt = str_printf("%s/%s.serial.txt", dir, s->name ? s->name : "unnamed");

        /* what came after the last request belongs to the transcript */
        if (s->rx.n) {
            r->cur = s;
            flush_rx(r);
            r->cur = NULL;
        }
        if (!raw || !txt || s->raw.bad || s->log.bad
            || write_file(raw, s->raw.p ? s->raw.p : "", s->raw.n)
            || write_file(txt, s->log.p ? s->log.p : "", s->log.n))
            bad = 1;
        free(raw);
        free(txt);
        free(s->name);
        free(s->raw.p);
        free(s->log.p);
        free(s->rx.p);
        free(s->data.p);
        n++;
    }
    free(r->s);
    free(r);
    return bad ? -1 : n;
}
