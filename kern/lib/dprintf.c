#ifdef DEBUG_MSG

#include <dev/console.h>
#include <dev/serial.h>

#include <lib/debug.h>
#include <lib/spinlock.h>
#include <lib/stdarg.h>

// CUSTOM
static spinlock_t dprintf_lk;
static spinlock_t vdprintf_lk;

struct dprintbuf
{
    int idx; /* current buffer index */
    int cnt; /* total bytes printed so far */
    char buf[CONSOLE_BUFFER_SIZE];
};

static void
cputs (const char *str)
{
    while (*str)
    {
        cons_putc (*str);
        str += 1;
    }
}

static void
putch (int ch, struct dprintbuf *b)
{
    b->buf[b->idx++] = ch;
    if (b->idx == CONSOLE_BUFFER_SIZE - 1)
    {
        b->buf[b->idx] = 0;
        cputs (b->buf);
        b->idx = 0;
    }
    b->cnt++;
}

int
vdprintf (const char *fmt, va_list ap)
{
    // CUSTOM
    spinlock_acquire(&vdprintf_lk);

    struct dprintbuf b;

    b.idx = 0;
    b.cnt = 0;
    vprintfmt ((void*) putch, &b, fmt, ap);

    b.buf[b.idx] = 0;
    cputs (b.buf);

    // CUSTOM
    spinlock_release(&vdprintf_lk);

    return b.cnt;
}

int
dprintf (const char *fmt, ...)
{
    // CUSTOM
    spinlock_acquire(&dprintf_lk);

    va_list ap;
    int cnt;

    va_start(ap, fmt);
    cnt = vdprintf (fmt, ap);
    va_end(ap);

    // CUSTOM
    spinlock_release(&dprintf_lk);

    return cnt;
}

#endif /* DEBUG_MSG */
