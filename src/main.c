#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
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

/* qtc_decode.c */
extern int32_t qtc_decode(uint8_t **, uint8_t *);
/* qtc_encode.c */
extern bool qtc_encode(FILE *, uint8_t *, size_t);

void show_help(int);
char *ch_ext(char *, char *);

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

		/* open QTC2 file */
		src_fd = fopen(argv[0], "rb");
		if(!src_fd) {
			fprintf(stderr, "fopen() returned NULL\n");
			return 1;
		}

		/* file size */
		fseek(src_fd, 0, SEEK_END);
		size_t qtc_size = ftell(src_fd);
		fseek(src_fd, 0, SEEK_SET);

		/* allocating buffer for QTC2 file data */
		uint8_t *qtc_data = malloc(qtc_size);
		if(!qtc_data) {
			fclose(src_fd);
			fprintf(stderr, "malloc() returned NULL\n");
			return 1;
		}
		fread(qtc_data, 1, qtc_size, src_fd);
		fclose(src_fd);

		/* decode */
		uint8_t *raw_data;
		int32_t raw_size = qtc_decode(&raw_data, qtc_data);
		free(qtc_data);
		if(raw_size == -1) {
			fprintf(stderr, "qtc_decode() returned -1\n");
			return 1;
		}
		
		/* write decoded data */
		dest_fd = fopen(argc < 2 ? ch_ext(argv[0], "raw") : argv[1], "wb");
		if(!dest_fd) {
			free(raw_data);
			fprintf(stderr, "fopen() returned NULL\n");
			return 1;
		}
		fwrite(raw_data, 1, raw_size, dest_fd);
		fclose(dest_fd);

		free(raw_data);

	} else if(action == ACTION_RAW_TO_QTC) {

		/* open raw file */
		src_fd = fopen(argv[0], "rb");
		if(!src_fd) {
			fprintf(stderr, "fopen() returned NULL\n");
			return 1;
		}

		/* file size */
		fseek(src_fd, 0, SEEK_END);
		size_t raw_size = ftell(src_fd);
		fseek(src_fd, 0, SEEK_SET);

		/* allocating buffer for raw file data */
		uint8_t *raw_data = malloc(raw_size);
		if(!raw_data) {
			fclose(src_fd);
			fprintf(stderr, "malloc() returned NULL\n");
			return 1;
		}
		fread(raw_data, 1, raw_size, src_fd);
		fclose(src_fd);

		/* open destination file */
		dest_fd = fopen(argc < 2 ? ch_ext(argv[0], "qtc") : argv[1], "wb");
		if(!dest_fd) {
			free(raw_data);
			fprintf(stderr, "fopen() returned NULL\n");
			return 1;
		}

		/* decode */
		bool qtc_size = qtc_encode(dest_fd, raw_data, raw_size);
		fclose(dest_fd);
		free(raw_data);
		if(!qtc_size) {
			fprintf(stderr, "qtc_encode() returned -1\n");
			return 1;
		}

	} else {

		fprintf(stderr, "Specify action first.\n");
		return 1;

	}

	return 0;
}
