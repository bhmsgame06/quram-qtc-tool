#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

enum {
	ACTION_UNDEFINED,
	ACTION_RAW_TO_QTC,
	ACTION_QTC_TO_RAW
};

static char *program_name;

const struct option longopts[] = {
	{"help",   0, NULL, 'h'},
	{"decode", 0, NULL, 'd'},
	{"encode", 0, NULL, 'e'},
	{NULL, 0, NULL, 0}
};

static char *new_filename;
static int action = ACTION_UNDEFINED;

/* print help to the terminal */
void show_help(int err) {
	fprintf(err == 1 ? stderr : stdout,
			"Usage: %s [options] <-d|-e> <in_file> [out_file] ...\n" \
			"\n" \
			"Available options:\n" \
			"  -h, --help   - print help and exit.\n" \
			"  -d, --decode - decode QTC.\n" \
			"  -e, --encode - encode to QTC.\n",
			program_name);
}

/* change extension of the filename */
char *ch_ext(char *filename, char *new_ext) {
	if(new_filename) free(new_filename);

	int len = strlen(filename);

	int i;
	for(i = len - 1; i > 0; i--) {
		if(filename[i] == '.') break;
	}
	if(i == 0) i = len;

	new_filename = malloc(i + strlen(new_ext) + 2);
	memcpy(new_filename, filename, i);
	new_filename[i] = '.';
	strcpy(new_filename + i + 1, new_ext);

	return new_filename;
}

/* main function */
int main(int argc, char *argv[]) {
	/* checking zeroth arg */
	if(argv[0] == NULL)
		program_name = "imtool";
	else
		program_name = argv[0];

	/* parsing arguments */
	int c;
	while((c = getopt_long(argc, argv, "hde", longopts, NULL)) != -1) {
		switch(c) {
			case 'h':
				show_help(0);
				return 0;

			case 'd':
				action = ACTION_QTC_TO_RAW;
				break;

			case 'e':
				action = ACTION_RAW_TO_QTC;
				break;

			default:
				show_help(1);
				return 1;
		}
	}
	
	argv += optind;
	argc -= optind;

	if(argc < 1) {
		show_help(1);
		return 1;
	}

	/* parsing done */

	FILE *src_fd, *dest_fd;

	if(action == ACTION_QTC_TO_RAW) {

		fprintf(stderr, "Not implemented\n");
		return 1;

	} else if(action == ACTION_RAW_TO_QTC) {

		fprintf(stderr, "Not implemented\n");
		return 1;

	} else {

		fprintf(stderr, "Specify action first.\n");
		return 1;

	}

	return 0;
}
