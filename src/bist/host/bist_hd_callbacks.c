/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "bist_hd_callbacks.h"

#include <string.h>

static bist_hd_callbacks_t s_cbs;

void bist_hd_register_callbacks(const bist_hd_callbacks_t *cbs)
{
    if (cbs == NULL) {
        memset(&s_cbs, 0, sizeof(s_cbs));
        return;
    }
    s_cbs = *cbs;
}

const bist_hd_callbacks_t *bist_hd_get_callbacks(void)
{
    return &s_cbs;
}
