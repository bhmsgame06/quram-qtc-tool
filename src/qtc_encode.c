#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

/* growing up the buffer */
#define REALLOC_INCREMENT	256

/* distance bits */
#define N_STD_DIST_BITS 6
#define N_EXT_DIST_BITS 12

/* min macro */
#define MIN(x, y)						(((x) < (y)) ? (x) : (y))

/* macro-block offset macros */
#define MBK_OFFS						((curr_mbk << 1) + curr_mbk_wrd)

/* write command bits macros */
#define WRITE_CMD_BITS(value, length)	{ \
											int l = (length); \
											while(true) { \
												int shift = 8 - ((t_cmd_bit & 7) + l); \
												if(shift < 0) { \
													t_cmd_buf[t_cmd_bit >> 3] |= (value) >> -shift; \
													int rem = 8 - (t_cmd_bit & 7); \
													t_cmd_bit += rem; \
													l -= rem; \
												} else { \
													t_cmd_buf[t_cmd_bit >> 3] |= (value) << shift; \
													t_cmd_bit += l; \
													break; \
												} \
											} \
										}

/* write to block array */
#define WRITE_BLK_BYTE(value)			t_block[t_block_length++] = (value);

/* copy from input to block array */
#define COPY_TO_BLK_ARRAY(length)		memcpy(&t_block[t_block_length], in, (length)); \
										t_block_length += length;

/* diff table */
extern const uint16_t QuramDDC_diffTable[256];

/* QTC2 encode */
bool qtc_encode(FILE *fd, uint8_t *in, size_t in_size) {
	/* encode status */
	int32_t status = true;

	/* buffers */
	uint8_t *p_cmd_buf = NULL;
	int cmd_bit = 0;
	int cmd_alloc_peak = 0;

	uint8_t *p_block = NULL;
	int block_length = 0;
	int block_alloc_peak = 0;

	/* temporary buffers */
	uint8_t t_cmd_buf[9];
	int t_cmd_bit;

	uint8_t t_block[17];
	int t_block_length;

	/* offset */
	uint32_t offset = 4;

	/* current decoded macro-blocks */
	int enc_mblks = 0;
	/* total macro-blocks */
	int total_mblks = (in_size >> 4) << 2;

	/* start encoding */
	while(enc_mblks < total_mblks) {

		memset(t_cmd_buf, 0, sizeof(t_cmd_buf));
		t_cmd_bit = cmd_bit & 7;

		if(enc_mblks < 4) {

			t_block_length = 0;

			for(int curr_mbk = 0; curr_mbk < 4; curr_mbk++) {
				/* first macro-block */
				if(enc_mblks == 0) {

					COPY_TO_BLK_ARRAY(4);
					in += 4;

				} else {

					WRITE_CMD_BITS(1, 1);

					for(int curr_mbk_wrd = 0; curr_mbk_wrd < 2; curr_mbk_wrd++) {

						uint16_t valcur = *(uint16_t *)in;
						uint16_t valoff = *(uint16_t *)(in - offset);

						if(valcur == valoff) {
							/* copy from offset */
							WRITE_CMD_BITS(0b1, 1);
						} else {
							/* walking through diff table */
							for(int diff_index = 0; diff_index < (sizeof(QuramDDC_diffTable) >> 1); diff_index++) {
								if((valcur ^ valoff) == QuramDDC_diffTable[diff_index]) {
									WRITE_CMD_BITS(0b00, 2);
									WRITE_BLK_BYTE(diff_index);
									goto skip_literal_1;
								}
							}
							WRITE_CMD_BITS(0b01, 2);
							COPY_TO_BLK_ARRAY(2);
						}
skip_literal_1:
						in += 2;

					}

				}

				enc_mblks++;
			}

		} else {

			uint32_t best_bit_count = UINT_MAX;
			uint32_t best_offset = 0;
			int curr_offset = 2;
			bool trying = true;

			uint8_t *old_in = in;
			uint32_t old_enc_mblks = enc_mblks;
			uint32_t old_offset = offset;

			int max_offset = MIN(enc_mblks << 2, (1 << N_EXT_DIST_BITS));

			for(; (curr_offset < max_offset) && trying; curr_offset++) {
encode_with_offset:
				in = old_in;
				enc_mblks = old_enc_mblks;

				/* resetting cmd */
				if(!trying) {
					memset(t_cmd_buf, 0, sizeof(t_cmd_buf));
					t_cmd_bit = (cmd_bit & 7);
				} else {
					t_cmd_bit = 0;
				}

				/* mixed or not */
				if(curr_offset != old_offset || memcmp(in, in - curr_offset, 16)) {

					if(!trying) {
						/* mixed */
						WRITE_CMD_BITS(0b1, 1);
						/* resetting block (and writing empty stdlzbits value to it) */
						t_block[0] = 0;
					} else {
						t_cmd_bit++;
					}
	
					t_block_length = 1;

					for(int curr_mbk = 0; curr_mbk < 4; curr_mbk++) {
	
						if(curr_offset != old_offset) {
							// sameoffs = false
							if(!trying) {
								if(curr_offset >= (1 << N_STD_DIST_BITS)) {
									WRITE_CMD_BITS(0b00000000000000 | curr_offset, 14);
								} else {
									WRITE_CMD_BITS(0b01000000       | curr_offset, 8);
								}
							} else {
								if(curr_offset >= (1 << N_STD_DIST_BITS)) {
									t_cmd_bit += 14;
								} else {
									t_cmd_bit += 8;
								}
							}
							offset = curr_offset;
						} else {
							// sameoffs = true
							if(!trying) {
								WRITE_CMD_BITS(0b1, 1);
							} else {
								t_cmd_bit++;
							}
							offset = old_offset;
						}
	
						for(int curr_mbk_wrd = 0; curr_mbk_wrd < 2; curr_mbk_wrd++) {
		
							uint16_t valcur = *(uint16_t *)in;
							uint16_t valoff = *(uint16_t *)(in - offset);
	
							if(valcur == valoff) {
								/* copy from offset */
								if(!trying) t_block[0] |= (1 << (7 - MBK_OFFS));
							} else {
								/* walking through diff table */
								for(int diff_index = 0; diff_index < (sizeof(QuramDDC_diffTable) >> 1); diff_index++) {
									if((valcur ^ valoff) == QuramDDC_diffTable[diff_index]) {

										if(!trying) {
											WRITE_CMD_BITS(0b0, 1);
											WRITE_BLK_BYTE(diff_index);
										} else {
											t_cmd_bit++;
											t_block_length++;
										}

										goto skip_literal_2;
									}
								}

								if(!trying) {
									WRITE_CMD_BITS(0b1, 1);
									COPY_TO_BLK_ARRAY(2);
								} else {
									t_cmd_bit++;
									t_block_length += 2;
								}

							}
skip_literal_2:
							in += 2;
	
						}
	
						if(!trying) enc_mblks++;
	
					}

				} else {

					/* resetting block */
					t_block_length = 0;

					/* not mixed */
					if(!trying) {
						WRITE_CMD_BITS(0b01, 2);
						enc_mblks += 4;
					} else {
						t_cmd_bit += 2;
					}
					in += 16;

				}

				/* comparing result bit count with the last one */
				uint32_t bit_count = (t_cmd_bit - (trying ? 0 : (cmd_bit & 7))) + (t_block_length << 3);
				if(bit_count < best_bit_count) {
					best_bit_count = bit_count;
					best_offset = curr_offset;
				}

			}

			if(trying) {
				trying = false;
				curr_offset = offset = best_offset;
				goto encode_with_offset;
			}

			/* 130 cuz (16 bytes) * (8 bit) + 2 bit */
			if(best_bit_count > 130) {
				/* writing mixed = 0, literal = 0 */
				memset(t_cmd_buf, 0, sizeof(t_cmd_buf));
				t_cmd_bit = (cmd_bit & 7) + 2;

				/* copying 16 raw bytes to block array */
				t_block_length = 16;
				memcpy(t_block, old_in, 16);

				/* restoring old offset */
				offset = old_offset;

				in = old_in + 16;
				enc_mblks = old_enc_mblks + 4;

			}

		}

		/* copying temporary buffer data to current buffers */
		
		/* cmd */
		cmd_bit += t_cmd_bit - (cmd_bit & 7);
		while((cmd_bit >> 3) + ((cmd_bit & 7) != 0) >= cmd_alloc_peak) {
			int old = cmd_alloc_peak;
			if((p_cmd_buf = realloc(p_cmd_buf, cmd_alloc_peak += REALLOC_INCREMENT)) == NULL) {
				status = false;
				goto end;
			}
			memset(p_cmd_buf + old, 0, REALLOC_INCREMENT);
		}
		for(int i = cmd_bit >> 3, k = t_cmd_bit >> 3; k >= 0; i--, k--) {
			p_cmd_buf[i] |= t_cmd_buf[k];
		}

		/* block */
		block_length += t_block_length;
		while(block_length >= block_alloc_peak) {
			int old = block_alloc_peak;
			if((p_block = realloc(p_block, block_alloc_peak += REALLOC_INCREMENT)) == NULL) {
				status = false;
				goto end;
			}
			memset(p_block + old, 0, REALLOC_INCREMENT);
		}
		for(int i = block_length - 1, k = t_block_length - 1; k >= 0; i--, k--) {
			p_block[i] = t_block[k];
		}

	}

	uint32_t cmd_length = (cmd_bit >> 3) + ((cmd_bit & 7) != 0);
	uint32_t extra_length = in_size - (total_mblks << 2);

	int32_t out_size = 16 + cmd_length + block_length + extra_length;

	fwrite(&out_size, 4, 1, fd);
	fprintf(fd, "QTC2");
	fwrite(&in_size, 4, 1, fd);
	fwrite(&cmd_length, 4, 1, fd);
	fwrite(p_cmd_buf, 1, cmd_length, fd);
	fwrite(p_block, 1, block_length, fd);
	fwrite(in, 1, extra_length, fd);
	
end:

	free(p_cmd_buf);
	free(p_block);

	return status;
}
