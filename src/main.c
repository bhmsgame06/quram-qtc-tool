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
	ACTION_QTC_TO_RAW,
	ACTION_BUILD_MAP
};

static char *program_name;

const struct option longopts[] = {
	{"help",        0, NULL, 'h'},
	{"output",      1, NULL, 'o'},
	{"decode",      0, NULL, 'd'},
	{"encode",      0, NULL, 'e'},
	{"build-map",   0, NULL, 'm'},
	{NULL, 0, NULL, 0}
};

static int action = ACTION_UNDEFINED;
static char output_file[256];

/* qtc_decode.c */
extern int32_t qtc_decode(uint8_t **, uint8_t *);
/* qtc_encode.c */
extern bool qtc_encode(FILE *, uint8_t *, size_t);
/* build_map.c */
extern int32_t build_map(char *, char *);

void show_help(int);
char *ch_ext(char *, char *);

/* print help to the terminal */
void show_help(int err) {
	fprintf(err == 1 ? stderr : stdout,
			"Usage: %s [options] [argv] ...\n" \
			"\n" \
			"Available options:\n" \
			"  -h, --help      - print help and exit.\n" \
			"\n" \
			"  -o, --output    - set output file.\n" \
			"\n" \
			"  -d, --decode    - action: decode QTC.\n" \
			"                    argv: <qtc_file>.\n" \
			"\n" \
			"  -e, --encode    - action: encode to QTC.\n" \
			"                    argv: <raw_file_1> [raw_file_2] [raw_file_3] ...\n" \
			"\n" \
			"  -m, --build-map - action: build map from text data.\n" \
			"                    argv: <raw_file>.\n" \
			"\n",
			program_name);
}

/* change extension of the filename */
char *ch_ext(char *filename, char *new_ext) {
	strncpy(output_file, filename, sizeof(output_file) - 1);
	char *dot = strrchr(output_file, '.');
	if(dot) *(uint8_t *)(dot + 1) = '\0';
	strncat(output_file, new_ext, sizeof(output_file) - 1);
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
	while((c = getopt_long(argc, argv, "ho:dem", longopts, NULL)) != -1) {
		switch(c) {
			case 'h':
				show_help(0);
				return 0;

			case 'o':
				strncpy(output_file, optarg, sizeof(output_file));
				break;

			case 'd':
				action = ACTION_QTC_TO_RAW;
				break;

			case 'e':
				action = ACTION_RAW_TO_QTC;
				break;

			case 'm':
				action = ACTION_BUILD_MAP;
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

		/* changing extension of an output file */
		if(!strlen(output_file)) ch_ext(argv[0], "raw");

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

		/* qtc data pointer */
		uint8_t *p_qtc = qtc_data;
		uint8_t *p_qtc_end = qtc_data + qtc_size;

		/* temporary filename */
		char tmp_filename[256];

		for(int i = 0; p_qtc < p_qtc_end; i++) {
			/* decode */
			uint8_t *raw_data;
			int32_t raw_size = qtc_decode(&raw_data, p_qtc);
			if(raw_size == -1) {
				fprintf(stderr, "qtc_decode() returned -1\n");
				return 1;
			}
			
			/* renaming an output file */
			strncpy(tmp_filename, output_file, sizeof(tmp_filename));
			char *dot = strrchr(tmp_filename, '.');
			snprintf(dot, (char *)&tmp_filename - dot + (sizeof(tmp_filename) - 1), "_%d.raw", i);

			/* write decoded data */
			dest_fd = fopen(tmp_filename, "wb");
			if(!dest_fd) {
				free(raw_data);
				fprintf(stderr, "fopen() returned NULL\n");
				return 1;
			}
			fwrite(raw_data, 1, raw_size, dest_fd);
			fclose(dest_fd);

			free(raw_data);

			p_qtc += *(uint32_t *)p_qtc;
		}

		free(qtc_data);

	} else if(action == ACTION_RAW_TO_QTC) {

		/* open destination file */
		if(!strlen(output_file)) ch_ext(argv[0], "qtc");
		dest_fd = fopen(output_file, "wb");
		if(!dest_fd) {
			fprintf(stderr, "fopen() returned NULL\n");
			return 1;
		}

		for(int i = 0; i < argc; i++) {
			/* open raw file */
			src_fd = fopen(argv[i], "rb");
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

			/* encode */
			bool qtc_size = qtc_encode(dest_fd, raw_data, raw_size);
			free(raw_data);
			if(!qtc_size) {
				fprintf(stderr, "qtc_encode() returned -1\n");
				return 1;
			}
		}

		fclose(dest_fd);

	} else if(action == ACTION_BUILD_MAP) {
		
		if(!strlen(output_file)) ch_ext(argv[0], "map");
		return build_map(output_file, argv[0]);

	} else {

		fprintf(stderr, "Specify action first.\n");
		return 1;

	}

	return 0;
}
