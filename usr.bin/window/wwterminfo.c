/*	$OpenBSD: wwterminfo.c,v 1.9 2001/07/09 07:04:58 deraadt Exp $	*/

/*
 * Copyright (c) 1982, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Wang at The University of California, Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
#if 0
static char sccsid[] = "@(#)wwterminfo.c	8.1 (Berkeley) 6/6/93";
#else
static char rcsid[] = "$OpenBSD: wwterminfo.c,v 1.9 2001/07/09 07:04:58 deraadt Exp $";
#endif
#endif /* not lint */

#ifdef TERMINFO

#include "ww.h"
#include <stdio.h>
#include <paths.h>
#include "local.h"

/*
 * Terminfo support
 *
 * Written by Brian Buhrow
 *
 * Subsequently modified by Edward Wang
 */

/*
 * Initialize the working terminfo directory
 */
wwterminfoinit()
{
	FILE *fp;
	char buf[2048];

		/* make the directory */
	(void) snprintf(wwterminfopath, sizeof wwterminfopath,
	    "%swwinXXXXXXXXXX", _PATH_TMP);
	if (mkdtemp(wwterminfopath) == NULL) {
		wwerrno = WWE_SYS;
		return -1;
	}
	(void) setenv("TERMINFO", wwterminfopath, 1);
		/* make a termcap entry and turn it into terminfo */
	(void) snprintf(buf, sizeof buf, "%s/cap", wwterminfopath);
	if ((fp = fopen(buf, "w")) == NULL) {
		wwerrno = WWE_SYS;
		return -1;
	}
	(void) fprintf(fp, "%sco#%d:li#%d:%s\n",
		WWT_TERMCAP, wwncol, wwnrow, wwwintermcap);
	(void) fclose(fp);
	(void) snprintf(buf, sizeof buf,
	    "cd %s; %s cap >info 2>/dev/null; %s info >/dev/null 2>&1",
	    wwterminfopath, _PATH_CAPTOINFO, _PATH_TIC);
	(void) system(buf);		/* XXX */
	return 0;
}

/*
 * Delete the working terminfo directory at shutdown
 */
wwterminfoend()
{

	switch (vfork()) {
	case -1:
		/* can't really do (or say) anything about errors */
		return -1;
	case 0:
		execl(_PATH_RM, _PATH_RM, "-rf", wwterminfopath, (char *)NULL);
		_exit(0);
	default:
		wait(NULL);
		return 0;
	}
}

#endif /* TERMINFO */
