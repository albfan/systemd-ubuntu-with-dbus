/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#ifndef foojournalratelimithfoo
#define foojournalratelimithfoo

/***
  This file is part of systemd.

  Copyright 2011 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include "macro.h"
#include "util.h"

typedef struct JournalRateLimit JournalRateLimit;

JournalRateLimit *journal_rate_limit_new(usec_t interval, unsigned burst);
void journal_rate_limit_free(JournalRateLimit *r);
int journal_rate_limit_test(JournalRateLimit *r, const char *id, int priority, uint64_t available);

#endif
