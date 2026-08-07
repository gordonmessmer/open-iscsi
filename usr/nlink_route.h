/*
 * rtnetlink monitoring for network address changes
 *
 * Copyright (C) 2026 Gordon Messmer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * See the file COPYING included with this distribution for more details.
 */
#ifndef NLINK_ROUTE_H
#define NLINK_ROUTE_H

int nlink_route_init(void);
void nlink_route_handle(int fd);
void nlink_route_close(int fd);

#endif
