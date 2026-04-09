#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* get total number of strings in data array, a.k.a.
 * number of NUL bytes. */
static int get_num_strings(char *data, int length) {
	int count = 0;
	for(int i = 0; i < length; i++) {
		if(!data[i]) count++;
	}
	return count;
}

/* rebuild map function */
int rebuild_map(char *file_out, char *file_in) {
	FILE *fd;

	/* reading a file */
	fd = fopen(file_in, "rb");
	if(!fd) {
		fprintf(stderr, "fopen() returned null\n");
		return 1;
	}

	fseek(fd, 0, SEEK_END);
	size_t data_size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	char *data = malloc(data_size);
	if(!data) {
		fclose(fd);
		fprintf(stderr, "malloc() returned NULL\n");
		return 1;
	}

	fread(data, 1, data_size, fd);
	fclose(fd);

	/* making a map */
	int num_strings = get_num_strings(data, data_size);
	uint32_t map[num_strings];

	uint32_t offset = 0;
	uint32_t string_index = 0;
	for(int i = 0; i < data_size; i++) {
		if(!data[i]) {
			map[string_index++] = offset;
			offset = i + 1;
		}
	}

	/* writing to a file */
	fd = fopen(file_out, "wb");
	if(!fd) {
		fprintf(stderr, "fopen() returned null\n");
		return 1;
	}

	fwrite(map, 4, num_strings, fd);
	fclose(fd);

	return 0;
}
