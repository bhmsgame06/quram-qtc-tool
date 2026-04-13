/* 
 * Original code by raulmrio28-git at
 * https://github.com/raulmrio28-git/ImrcUnpacker.git
 *
 * This code is a refactored (by me) version of the original code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* distance bits */
#define N_STD_DIST_BITS 6
#define N_EXT_DIST_BITS 12

/* extract bits macro */
#define EXTRACT_BITS(value, bits)	while(bit < bits) { \
										cmd_bytes = (cmd_bytes << 8) | *p_cmd_buf++; \
										bit += 8; \
									} \
									bit -= bits; \
									value = ((cmd_bytes >> bit) & ((1 << bits) - 1));

/* macro-block offset macro */
#define MBK_OFFS					((curr_mbk << 1) + curr_mbk_wrd)

/* diff table */
extern const uint16_t QuramDDC_diffTable[256];

/* QTC2 decode */
int32_t qtc_decode(uint8_t **p_out, uint8_t *in, int *p_percentage) {
	/* checking magic */
	if(memcmp(in + 4, "QTC2", 4)) {
		fprintf(stderr, "Not a QTC2 file\n");
		return -1;
	}

	/* output size */
	uint32_t out_size = in[8] |
			(in[9] << 8) |
			(in[10] << 16) |
			(in[11] << 24);
	/* output buffer */
	uint8_t *out_buf = malloc(out_size);
	if(!out_buf) {
		fprintf(stderr, "malloc() returned NULL\n");
		return -1;
	}

	/* incrementable output pointer */
	uint8_t *p_curr_out = (uint8_t *)out_buf;

	/* we will hold 2 streams in memory;
	 * this */
	uint8_t *p_cmd_buf = in + 16;
	/* and this */
	uint8_t *p_in_block = in + 
		(in[12] | 
		 (in[13] << 8) | 
		 (in[14] << 16) | 
		 (in[15] << 24)) + 16;

	/* bit index */
	uint32_t bit = 0;
	/* command bytes */
	uint32_t cmd_bytes = 0;

	/* offset */
	uint32_t offset = 4;

	/* current decoded macro-blocks */
	int unp_mblks = 0;
	/* total macro-blocks */
	int total_mblks = (out_size >> 4) << 2;

	/* starting decoding;
	 * first quad-macro-block */
	for(int curr_mbk = 0; curr_mbk < 4; curr_mbk++) {

		/* the first macro-block (4 bytes) will be taken 
		 * as a raw value from input and be written to output */
		if(!curr_mbk) {

			memcpy(p_curr_out, p_in_block, 4);
			p_curr_out += 4;
			p_in_block += 4;

		} else {

			/* read new offset if bit is set */
			bool sameoffs;
			EXTRACT_BITS(sameoffs, 1);
			if(!sameoffs) {
				bool stdoffs;
				EXTRACT_BITS(stdoffs, 1);
				EXTRACT_BITS(offset, (stdoffs ? N_STD_DIST_BITS : N_EXT_DIST_BITS));
			}

			for(int curr_mbk_wrd = 0; curr_mbk_wrd < 2; curr_mbk_wrd++) {

				/* copy value from output current position minus offset
				 * if bit is set */
				bool stdlz;
				EXTRACT_BITS(stdlz, 1);
				if(stdlz) {
					*(uint16_t *)p_curr_out = *(uint16_t *)(p_curr_out - offset);
				} else {
					bool literal;
					EXTRACT_BITS(literal, 1);
					*(uint16_t *)p_curr_out = literal ? *(uint16_t *)p_in_block : (QuramDDC_diffTable[*p_in_block] ^ *(uint16_t *)(p_curr_out - offset));
					p_in_block += literal + 1;
				}
				p_curr_out += 2;
			}
		}

		unp_mblks++;

	}

	/* decoding other macro-blocks */
	while(unp_mblks < total_mblks) {

		/* mixed */
		bool mixed;
		EXTRACT_BITS(mixed, 1);
		if(mixed) {

			uint8_t stdlz_bits = *p_in_block++;
			for(int curr_mbk = 0; curr_mbk < 4; curr_mbk++) {

				/* read new offset if bit is set */
				bool sameoffs;
				EXTRACT_BITS(sameoffs, 1);
				if(!sameoffs) {
					bool extdoffs;
					EXTRACT_BITS(extdoffs, 1);
					EXTRACT_BITS(offset, (extdoffs ? N_STD_DIST_BITS : N_EXT_DIST_BITS));
				}

				for(int curr_mbk_wrd = 0; curr_mbk_wrd < 2; curr_mbk_wrd++) {

					if((stdlz_bits & (1 << (7 - MBK_OFFS)))) {
						*(uint16_t *)p_curr_out = *(uint16_t *)(p_curr_out - offset);
					} else {
						bool literal;
						EXTRACT_BITS(literal, 1);
						*(uint16_t *)p_curr_out = literal ? *(uint16_t *)p_in_block : (QuramDDC_diffTable[*p_in_block] ^ *(uint16_t *)(p_curr_out - offset));
						p_in_block += literal + 1;
					}

					p_curr_out += 2;
				}

			}

		} else {

			/* copy 16 bytes from output current position minus offset
			 * if bit is set, otherwise copy those from p_in_block */
			bool lz;
			EXTRACT_BITS(lz, 1);
			for(int curr_byte = 0; curr_byte < 16; curr_byte++) {

				if(lz) {
					*p_curr_out = *(p_curr_out - offset);
				} else {
					*p_curr_out = *p_in_block;
					p_in_block++;
				}

				p_curr_out++;

			}

		}

		unp_mblks += 4;

		*p_percentage = (int)((float)unp_mblks / (float)total_mblks * 100.0f);
	}

	/* final */
	int out_size_mul_of_16 = out_size & ~0x0f;
	if(out_size > out_size_mul_of_16)
		memcpy(p_curr_out, p_in_block, out_size - out_size_mul_of_16);

	*p_percentage = 100;

	*p_out = out_buf;

	return out_size;
}
