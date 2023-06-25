/*
 * FBPAD FRAMEBUFFER VIRTUAL TERMINAL
 *
 * Copyright (C) 2009-2021 Ali Gholami Rudi <ali at rudi dot ir>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <linux/fb.h>
#include "conf.h"
#include "fbpad.h"
#include "draw.h"

int verbose;
struct fb_bitfield *pixel_format;
static struct fb_bitfield rgba[4];
static struct term *term;

static int do_poll(void)
{
	struct pollfd pfd;
	pfd.fd = term_fd(term);
	pfd.events = POLLIN;
	if (poll(&pfd, 1, 1000) < 1)
		return 0;
	if (pfd.revents & POLLIN) {
		term_read();
	} else {
		term_end();
		return 1;
	}
	return 0;
}

static void signalreceived(int n)
{
	switch (n) {
	case SIGCHLD:
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
		break;
	}
}

static void usage(void)
{
	fprintf(stderr,
	"usage: fbpad [-v] [--rgba FORMAT] [COMMAND]\n"
	"\n"
	"  -v             print diagnostic info\n"
	"  --rgba FORMAT  override pixel format (see below)\n"
	"\n"
	"Normally pixel format is read with FBIOGET_VSCREENINFO ioctl, but it may lie.\n"
	"Override format with --rgba, where FORMAT is the same as output by fbset:\n"
	"\n"
	"  Rl/Ro,Gl/Go,Bl/Bo,Al/Ao\n"
	"  Rl, Gl, Bl, Al - length in bits of red, green, blue, alpha component\n"
	"  Ro, Go, Bo, Ao - offset in bits of red, green, blue, alpha component\n"
	"  0/0 means color component is not used (e.g. alpha)\n"
	"  E.g. 8/0,8/8,8/16,8/24 - pixels are 4-byte sequences of RGBA\n"
	"       8/16,8/8,8/0,8/24 - pixels are 4-byte sequences of BGRA\n"
	"       8/16,8/8,8/0,0/0  - pixels are 3-byte sequences of BGR (depth 24)\n"
	"                           or 4-byte BGRA (depth 32) with alpha ignored\n");
	exit(1);
}

static char **parse_args(char **argv)
{
	++argv;
	for (; argv[0] && argv[0][0] == '-'; ++argv) {
		if (!strcmp(argv[0], "-v"))
			verbose = 1;
		else if (!strcmp(argv[0], "--rgba")) {
			int ret;
			++argv;
			if (!argv[0]) {
				fprintf(stderr, "--rgba is missing FORMAT\n");
				usage();
			}
			ret = sscanf(argv[0], "%d/%d,%d/%d,%d/%d,%d/%d",
				&rgba[0].length, &rgba[0].offset,
				&rgba[1].length, &rgba[1].offset,
				&rgba[2].length, &rgba[2].offset,
				&rgba[3].length, &rgba[3].offset);
			if (ret != 8) {
				fprintf(stderr, "--rgba FORMAT is incorrect\n");
				usage();
			}
			pixel_format = rgba;
		} else
			usage();
	}
	return argv;
}

int main(int argc, char **argv)
{
	char *shell[8] = SHELL;
	argv = parse_args(argv);
	if (fb_init(getenv("FBDEV"))) {
		fprintf(stderr, "fbpad: failed to initialize the framebuffer\n");
		return 1;
	}
	if (pad_init()) {
		fprintf(stderr, "fbpad: cannot find fonts\n");
		return 1;
	}
	signal(SIGCHLD, signalreceived);
	term = term_make();
	term_load(term, 1);
	term_redraw(1);
	term_exec(argv[0] ? argv : shell, 0);
	while (!do_poll())
		;
	term_free(term);
	pad_free();
	fb_free();
	return 0;
}
