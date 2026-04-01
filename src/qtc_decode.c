/* 
 * Original code by raulmrio28-git at
 * https://github.com/raulmrio28-git/ImrcUnpacker.git
 *
 * This code is a refactored (by me) version of the original code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
const uint16_t QuramDDC_diffTable[256] = {
	/* 00000000 */	0x0001, 0x0003, 0x0100, 0x0002, 0x0008, 0x0007, 0x0006, 0x0300,
	/* 00000008 */	0x0010, 0x0004, 0x0200, 0x0009, 0x0040, 0x0018, 0x0005, 0x0020,
	/* 00000010 */	0x000c, 0x000e, 0x000f, 0x000a, 0x00c0, 0x0800, 0x0700, 0x0101,
	/* 00000018 */	0x0400, 0x000b, 0x0030, 0x0011, 0x0080, 0x0600, 0x000d, 0x0012,
	/* 00000020 */	0x001c, 0x0500, 0x001b, 0x001e, 0x0014, 0x001a, 0x0028, 0x0038,
	/* 00000028 */	0x1000, 0x001f, 0x0019, 0x0016, 0x0060, 0x2000, 0x0013, 0x001d,
	/* 00000030 */	0x0103, 0x0024, 0x0017, 0x0015, 0x0102, 0x01c0, 0x0f00, 0x003c,
	/* 00000038 */	0x0301, 0x0c00, 0x1800, 0x0048, 0x0021, 0x0034, 0x0e00, 0x0202,
	/* 00000040 */	0x002c, 0x0070, 0x0a00, 0x0303, 0x0036, 0x0201, 0x003f, 0x0d00,
	/* 00000048 */	0x0180, 0x003e, 0x3000, 0x0900, 0x0078, 0x0022, 0x0050, 0x003a,
	/* 00000050 */	0x0041, 0x0107, 0x0033, 0x0106, 0x0026, 0x002a, 0x00a0, 0x0023,
	/* 00000058 */	0x0029, 0x0088, 0x0044, 0x003d, 0x00e0, 0x0032, 0x002e, 0x0039,
	/* 00000060 */	0x0031, 0x002d, 0x00f0, 0x0140, 0x0b00, 0x003b, 0x0058, 0x4000,
	/* 00000068 */	0x0037, 0x0035, 0x0068, 0x0302, 0x007c, 0x002f, 0x0027, 0x0064,
	/* 00000070 */	0x0090, 0x0074, 0x0203, 0x0104, 0x006c, 0x1100, 0x03c0, 0x00ff,
	/* 00000078 */	0x0025, 0xf000, 0x1f00, 0x0701, 0x0042, 0x007f, 0x002b, 0x0105,
	/* 00000080 */	0x0054, 0x1c00, 0x004c, 0x0801, 0x0043, 0x6000, 0x005c, 0x007e,
	/* 00000088 */	0x00e8, 0x0108, 0x00f8, 0xe000, 0x0206, 0x1e00, 0x0380, 0x0061,
	/* 00000090 */	0x007a, 0x004e, 0x0601, 0x1001, 0x00c8, 0x8000, 0x1d00, 0x00d0,
	/* 00000098 */	0x0072, 0x0049, 0x1600, 0x1a00, 0x0046, 0x7000, 0x010f, 0x0110,
	/* 000000a0 */	0x0076, 0x1200, 0x1400, 0x0404, 0x0606, 0x010e, 0x00fc, 0x1700,
	/* 000000a8 */	0x006e, 0x00fe, 0x1300, 0x0062, 0x0066, 0xc000, 0x0204, 0x0306,
	/* 000000b0 */	0x0063, 0x0707, 0x0280, 0x0602, 0x0055, 0x0047, 0x006a, 0x010c,
	/* 000000b8 */	0x0052, 0x0501, 0x00d8, 0x0307, 0x0073, 0x0109, 0x0808, 0x0401,
	/* 000000c0 */	0x004a, 0x2020, 0x005a, 0x0702, 0x00b0, 0x0045, 0x0207, 0x0304,
	/* 000000c8 */	0x0402, 0x005e, 0x010a, 0x0079, 0x3800, 0x00f4, 0x1500, 0x01e0,
	/* 000000d0 */	0x1b00, 0x0071, 0x1010, 0x00c1, 0x00e4, 0x0502, 0x0056, 0x007d,
	/* 000000d8 */	0x0081, 0x0077, 0x00cc, 0x0703, 0x010d, 0x0205, 0x0340, 0x5000,
	/* 000000e0 */	0x0082, 0x0067, 0xff00, 0x0120, 0x0069, 0x0098, 0x00c3, 0x1900,
	/* 000000e8 */	0x0065, 0x007b, 0x0240, 0x0603, 0x00ec, 0x0059, 0x00fa, 0x0403,
	/* 000000f0 */	0x0075, 0x006f, 0x3100, 0x3300, 0x004f, 0x00b8, 0x006d, 0x0208,
	/* 000000f8 */	0x004d, 0x0111, 0x0051, 0x020e, 0x00dc, 0x00c4, 0x2100, 0x00a8 
};

/* QTC2 decode */
int32_t qtc_decode(uint8_t **p_out, uint8_t *in) {
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
	int total_mblks = 4 * (out_size >> 4);

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
	}

	/* final */
	int out_size_mul_of_16 = out_size & ~0x0f;
	if(out_size > out_size_mul_of_16)
		memcpy(p_curr_out, p_in_block, out_size - out_size_mul_of_16);

	*p_out = out_buf;

	return out_size;
}
