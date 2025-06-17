#include <stdio.h>
#include "bist_log.h"

extern unsigned char _heap_start, _heap_end;

/* libc calls this to get the current thread’s reent struct.
 * In a single‑threaded system we always return the one global copy. */
struct _reent *__getreent(void)
{
    return _GLOBAL_REENT;      // == &_impure_data by default
}

void *_sbrk(int incr)
{
    static unsigned char *cur = &_heap_start;
    unsigned char *prev = cur;
    if (cur + incr > &_heap_end) {
        ESP_LOGE("heap", "Heap out of memory");
        return (void *)-1;
    }
    cur += incr;
    return prev;
}

void *_sbrk_r(struct _reent *r, int incr)
{
    (void)r; // unused
    return _sbrk(incr);
}
