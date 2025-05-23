/*
 * Copyright (c) 2015 Red Hat.  All Rights Reserved.
 * Copyright (c) 2007 Silicon Graphics, Inc.  All Rights Reserved.
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

#include "pmapi.h"
#include <signal.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifndef IS_MINGW
static volatile sig_atomic_t finished;

static void
sigterm(int sig)
{
    (void)sig;
    finished = 1;
}

static void
sigchld(int sig)
{
    int sts;

    (void)sig;
    do {
	sts = waitpid(-1, NULL, WNOHANG);
	if (sts < 0 && errno == ECHILD)
	    finished = 1;
    } while (sts > 0);
}

static int
pmpause(void)
{
    sigset_t sigset;
    struct sigaction sigact;
    int i, finish[] = { SIGINT, SIGQUIT, SIGTERM };

    sigfillset(&sigset);
    sigdelset(&sigset, SIGCHLD);

    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = &sigchld;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sigact, 0) == -1) {
	perror("pmpause: 1st sigaction failed");
	return 1;
    }

    for (i = 0; i < sizeof(finish) / sizeof(int); i++) {
	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = &sigterm;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = SA_RESTART;
	if (sigaction(finish[i], &sigact, 0) == -1) {
	    perror("pmpause: 2nd sigaction failed");
	    return 1;
	}
	sigdelset(&sigset, finish[i]);
    }

    if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
	/* most unlikely */
	perror("pmpause: sigprocmask failed");
	return 1;
    }
    while (!finished)
	pause();
    return 0;
}
#else
static int
pmpause(void)
{
    fprintf(stderr, "Not yet supported\n");
    return 1;
}
#endif

static int
pmsleep(const char *interval)
{
    struct timespec rqt;
    char *msg;
    
    if (pmParseInterval(interval, &rqt, &msg) < 0) {
	fputs(msg, stderr);
	free(msg);
    } else {
	if (0 != nanosleep(&rqt, NULL))
	    return oserror();
    }
    return 0;
}

static void
usage(void)
{
    if (strcmp(pmGetProgname(), "pmpause") == 0)
	fprintf(stderr, "Usage: pmpause [-w where]\n");
    else
	fprintf(stderr, "Usage: pmsleep [-w where] interval\n");
}

int
main(int argc, char **argv)
{
    int sts = 1;

    pmSetProgname(argv[0]);

    if (argc >= 2 && (strcmp(argv[1], "-?") == 0 || strcmp(argv[1], "--help") == 0)) {
	usage();
    }
    else {
	if (strcmp(pmGetProgname(), "pmpause") == 0)
	    sts = pmpause();
	else if (argc >= 2)
	    sts = pmsleep(argv[argc-1]);
	else {
	    usage();
	}
    }

    exit(sts);
}
