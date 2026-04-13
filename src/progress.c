#include <stdio.h>
#include <unistd.h>
#include "progress.h"

/* start tracking the speed of transmission.
 * should be called in a separate thread */
void *progress_track_start(struct progress *ctx) {
	// start
	ctx->show_progress = 1;

	// animation
	char anim_c[4] = "|/-\\";
	int anim_n = 0;

	// local variables
	int n = 0;

	while(ctx->show_progress) {
		for(int i = 0; i < n; i++)
			write(1, " ", 1);

		n = printf("\r \033[1;94m%c\033[0m [%s]: %d%%\r",
				anim_c[anim_n = ((anim_n + 1) & 3)],
				ctx->filename,
				ctx->percentage);

		fflush(stdout);

		usleep(100000);
	}
}

/* stop tracking the speed of transmission */
void progress_track_stop(struct progress *ctx) {
	ctx->show_progress = 0;
}
