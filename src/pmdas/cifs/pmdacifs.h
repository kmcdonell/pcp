/*
 * Common Internet File System (CIFS) PMDA
 *
 * Copyright (c) 2014, 2025 Red Hat.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef PMDACIFS_H
#define PMDACIFS_H

#include "stats.h"

extern pmInDom cifs_indom(int);
#define INDOM(i) cifs_indom(i)

enum {
        CLUSTER_GLOBAL_STATS = 0,		/* /proc/fs/cifs/Stats - global cifs*/
	CLUSTER_FS_STATS = 1,			/* /proc/fs/cifs/Stats - per fs*/
	CLUSTER_DEBUG_GLOBAL_STATS = 2, 		/* /proc/fs/cifs/DebugData - global*/
	CLUSTER_DEBUG_SERVERS_STATS = 3,		/* /proc/fs/cifs/DebugData - per server*/
	CLUSTER_DEBUG_SESSIONS_STATS = 4,	/* /proc/fs/cifs/DebugData - per session*/
	CLUSTER_DEBUG_SHARES_STATS = 5,		/* /proc/fs/cifs/DebugData - per share*/
	NUM_CLUSTERS
};

enum {
	CIFS_FS_INDOM = 0,       	/* 0 -- mounted cifs filesystem names */
        CIFS_DEBUG_SVR_INDOM,
        CIFS_DEBUG_SESSION_INDOM,
        CIFS_DEBUG_SHARE_INDOM,
	NUM_INDOMS
};

struct cifs_fs {
        char name[PATH_MAX];
        struct fs_stats fs_stats;
};

extern unsigned int global_version_major;
extern unsigned int global_version_minor;

extern pmdaMetric metrictable[];
extern int metrictable_size();

extern char *strtrim(char *);

#endif	/*PMDACIFS_H*/
