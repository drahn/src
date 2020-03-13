/* $OpenBSD: term_tag.c,v 1.1 2020/03/13 00:31:05 schwarze Exp $ */
/*
 * Copyright (c) 2015,2016,2018,2019,2020 Ingo Schwarze <schwarze@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Functions to write a ctags(1) file.
 * For use by the mandoc(1) ASCII and UTF-8 formatters only.
 */
#include <sys/types.h>

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mandoc.h"
#include "roff.h"
#include "tag.h"
#include "term_tag.h"

static void tag_signal(int) __attribute__((__noreturn__));

static struct tag_files tag_files;


/*
 * Prepare for using a pager.
 * Not all pagers are capable of using a tag file,
 * but for simplicity, create it anyway.
 */
struct tag_files *
term_tag_init(char *tagname)
{
	struct sigaction	 sa;
	int			 ofd;	/* In /tmp/, dup(2)ed to stdout. */
	int			 tfd;

	ofd = tfd = -1;
	tag_files.tfs = NULL;
	tag_files.tcpgid = -1;
	tag_files.tagname = tagname;

	/* Clean up when dying from a signal. */

	memset(&sa, 0, sizeof(sa));
	sigfillset(&sa.sa_mask);
	sa.sa_handler = tag_signal;
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	/*
	 * POSIX requires that a process calling tcsetpgrp(3)
	 * from the background gets a SIGTTOU signal.
	 * In that case, do not stop.
	 */

	sa.sa_handler = SIG_IGN;
	sigaction(SIGTTOU, &sa, NULL);

	/* Save the original standard output for use by the pager. */

	if ((tag_files.ofd = dup(STDOUT_FILENO)) == -1) {
		mandoc_msg(MANDOCERR_DUP, 0, 0, "%s", strerror(errno));
		goto fail;
	}

	/* Create both temporary output files. */

	(void)strlcpy(tag_files.ofn, "/tmp/man.XXXXXXXXXX",
	    sizeof(tag_files.ofn));
	(void)strlcpy(tag_files.tfn, "/tmp/man.XXXXXXXXXX",
	    sizeof(tag_files.tfn));
	if ((ofd = mkstemp(tag_files.ofn)) == -1) {
		mandoc_msg(MANDOCERR_MKSTEMP, 0, 0,
		    "%s: %s", tag_files.ofn, strerror(errno));
		goto fail;
	}
	if ((tfd = mkstemp(tag_files.tfn)) == -1) {
		mandoc_msg(MANDOCERR_MKSTEMP, 0, 0,
		    "%s: %s", tag_files.tfn, strerror(errno));
		goto fail;
	}
	if ((tag_files.tfs = fdopen(tfd, "w")) == NULL) {
		mandoc_msg(MANDOCERR_FDOPEN, 0, 0, "%s", strerror(errno));
		goto fail;
	}
	tfd = -1;
	if (dup2(ofd, STDOUT_FILENO) == -1) {
		mandoc_msg(MANDOCERR_DUP, 0, 0, "%s", strerror(errno));
		goto fail;
	}
	close(ofd);
	return &tag_files;

fail:
	term_tag_unlink();
	if (ofd != -1)
		close(ofd);
	if (tfd != -1)
		close(tfd);
	if (tag_files.ofd != -1) {
		close(tag_files.ofd);
		tag_files.ofd = -1;
	}
	tag_files.tagname = NULL;
	return NULL;
}

void
term_tag_write(struct roff_node *n, size_t line)
{
	const char	*cp;
	int		 len;

	if (tag_files.tfs == NULL)
		return;
	if (n->string == NULL)
		n = n->child;
	cp = n->string;
	if (cp[0] == '\\' && (cp[1] == '&' || cp[1] == 'e'))
		cp += 2;
	len = strcspn(cp, " \t\\");
	fprintf(tag_files.tfs, "%.*s %s %zu\n",
	    len, cp, tag_files.ofn, line);
}

void
term_tag_finish(void)
{
	if (tag_files.tfs == NULL)
		return;
	fclose(tag_files.tfs);
	tag_files.tfs = NULL;
	switch (tag_check(tag_files.tagname)) {
	case TAG_EMPTY:
		unlink(tag_files.tfn);
		*tag_files.tfn = '\0';
		/* FALLTHROUGH */
	case TAG_MISS:
		if (tag_files.tagname == NULL)
			break;
		mandoc_msg(MANDOCERR_TAG, 0, 0, "%s", tag_files.tagname);
		tag_files.tagname = NULL;
		break;
	case TAG_OK:
		break;
	}
}

void
term_tag_unlink(void)
{
	pid_t	 tc_pgid;

	if (tag_files.tcpgid != -1) {
		tc_pgid = tcgetpgrp(tag_files.ofd);
		if (tc_pgid == tag_files.pager_pid ||
		    tc_pgid == getpgid(0) ||
		    getpgid(tc_pgid) == -1)
			(void)tcsetpgrp(tag_files.ofd, tag_files.tcpgid);
	}
	if (*tag_files.ofn != '\0') {
		unlink(tag_files.ofn);
		*tag_files.ofn = '\0';
	}
	if (*tag_files.tfn != '\0') {
		unlink(tag_files.tfn);
		*tag_files.tfn = '\0';
	}
	if (tag_files.tfs != NULL) {
		fclose(tag_files.tfs);
		tag_files.tfs = NULL;
	}
}

static void
tag_signal(int signum)
{
	struct sigaction	 sa;

	term_tag_unlink();
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sigaction(signum, &sa, NULL);
	kill(getpid(), signum);
	/* NOTREACHED */
	_exit(1);
}
