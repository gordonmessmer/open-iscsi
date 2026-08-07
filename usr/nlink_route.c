/*
 * rtnetlink monitoring for network address changes
 *
 * When an interface gains a new IP address, log into all
 * targets configured with startup=automatic that don't
 * already have a running session.
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "idbm.h"
#include "config.h"
#include "initiator.h"
#include "mgmt_ipc.h"
#include "log.h"
#include "iscsi_err.h"
#include "nlink_route.h"

static int login_automatic_rec(void *data __attribute__((unused)),
			       node_rec_t *rec)
{
	queue_task_t *qtask;
	int rc;

	if (rec->startup != ISCSI_STARTUP_AUTOMATIC)
		return 0;

	qtask = calloc(1, sizeof(*qtask));
	if (!qtask) {
		log_error("Could not allocate qtask for automatic login");
		return 0;
	}
	qtask->mgmt_ipc_fd = -1;
	qtask->allocated = 1;

	rc = session_login_task(rec, qtask);
	if (rc) {
		mgmt_ipc_write_rsp(qtask, rc);
		if (rc != ISCSI_ERR_SESS_EXISTS)
			log_debug(1, "Automatic login to %s failed: %d",
				  rec->name, rc);
	}

	return 0;
}

static void login_automatic_targets(void)
{
	int nr_found = 0;

	log_debug(1, "Network change detected, logging in automatic targets");
	idbm_for_each_rec(&nr_found, NULL, login_automatic_rec, false);
}

int nlink_route_init(void)
{
	int fd;
	struct sockaddr_nl addr;

	fd = socket(AF_NETLINK, SOCK_DGRAM | SOCK_NONBLOCK, NETLINK_ROUTE);
	if (fd < 0) {
		log_error("Could not create rtnetlink socket: %s",
			  strerror(errno));
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		log_error("Could not bind rtnetlink socket: %s",
			  strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

void nlink_route_handle(int fd)
{
	char buf[4096];
	struct nlmsghdr *nlh;
	ssize_t len;
	bool new_addr = false;

	while ((len = recv(fd, buf, sizeof(buf), 0)) > 0) {
		for (nlh = (struct nlmsghdr *)buf;
		     NLMSG_OK(nlh, (size_t)len);
		     nlh = NLMSG_NEXT(nlh, len)) {
			if (nlh->nlmsg_type == RTM_NEWADDR)
				new_addr = true;
		}
	}

	if (len < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
		log_error("Error reading rtnetlink: %s", strerror(errno));

	if (new_addr)
		login_automatic_targets();
}

void nlink_route_close(int fd)
{
	if (fd >= 0)
		close(fd);
}
