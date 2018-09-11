#define BAD8 0xEF, 0xBF, 0xBD

	// 1 byte

	// 0xxx.xxxx
	TEST( 1_ok_1a, 0, 0, 0,
        AR({ 0, 'Z' }),
        AR({ 0, 'Z' })
    )

	TEST( 1_ok_1b, 0, 0, 0,
		AR({ 0x7F, 'Z' }),
		AR({ 0x7F, 'Z' })
	)
	// 10xx.xxxx - invalid
	TEST( 1_inv_1a, -1, 0, 0,
		AR({ 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 1_inv_1b, -1, 0, 0,
		AR({ 0xBF, 'Z' }),
		AR({ BAD8, 'Z' })
	)

	// 2 byte
	// 110x.xxxx 10xx.xxxx

    // 1100.0000 1000.0000 - 000000 (1 byte) - (0xC0,0xC1 are invalid) - invalid
    // 1100.0001 1011.1111 - 00007F - invalid
	TEST( 2_inv_1a, -1, 0, 0,
		AR({ 0xC0, 0x80, 'Z' }),
		AR({ BAD8, BAD8, 'Z' })
	)
	TEST( 2_inv_1b, -1, 0, 0,
		AR({ 0xC1, 0xBF, 'Z' }),
		AR({ BAD8, BAD8, 'Z' })
	)
    // 1100.0010 1000.0000 - 000080
    // 1101.1111 1011.1111 - 0007FF
	TEST( 2_ok_1a, 0, 0, 0,
		AR({ 0xC2, 0x80, 'Z' }),
		AR({ 0xC2, 0x80, 'Z' })
	)
	TEST( 2_ok_1b, 0, 0, 0,
		AR({ 0xDF, 0xBF, 'Z' }),
		AR({ 0xDF, 0xBF, 'Z' })
	)
    // partial
	TEST( 2_part_1, -1, 0, 0,
		AR({ 0xC2 }),
		AR({ BAD8 })
	)
	TEST( 2_partinv_1, -1, 0, 0,
		AR({ 0xC2, 'Z' }),
		AR({ BAD8, 'Z' })
	)

	// 3 bytes
	// 1110.xxxx 10xx.xxxx 10xx.xxxx

    // 1110.0000 1000.0000 1000.0000 - 000000 (1 byte) - invalid
    // 1110.0000 1000.0001 1011.1111 - 00007F - invalid
	TEST( 3_inv_1a, -1, 0, 0,
		AR({ 0xE0, 0x80, 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 3_inv_1b, -1, 0, 0,
		AR({ 0xE0, 0x81, 0xBF, 'Z' }),
		AR({ BAD8, 'Z' })
	)
    // 1110.0000 1000.0010 1000.0000 - 000080 (2 bytes) - invalid
    // 1110.0000 1001.1111 1011.1111 - 0007FF - invalid
	TEST( 3_inv_2a, -1, 0, 0,
		AR({ 0xE0, 0x82, 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 3_inv_2b, -1, 0, 0,
		AR({ 0xE0, 0x9F, 0xBF, 'Z' }),
		AR({ BAD8, 'Z' })
	)
    // 1110.0000 1010.0000 1000.0000 - 000800
    // 1110.1111 1011.1111 1011.1111 - 00FFFF
	TEST( 3_ok_1a, 0, 0, 0,
		AR({ 0xE0, 0xA0, 0x80, 'Z' }),
		AR({ 0xE0, 0xA0, 0x80, 'Z' })
	)
	TEST( 3_ok_1b, 0, 0, 0,
		AR({ 0xEF, 0xBF, 0xBF, 'Z' }),
		AR({ 0xEF, 0xBF, 0xBF, 'Z' })
	)
    // partial
	TEST( 3_part_1, -1, 0, 0,
		AR({ 0xE0 }),
		AR({ BAD8 })
	)
	TEST( 3_part_2, -1, 0, 0,
		AR({ 0xE0, 0x80 }),
		AR({ BAD8 })
	)
	TEST( 3_partinv_1, -1, 0, 0,
		AR({ 0xE0, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 3_partinv_2, -1, 0, 0,
		AR({ 0xE0, 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)

	// 4 bytes
	// 1111.0xxx 10xx.xxxx 10xx.xxxx 10xx.xxxx

    // 1111.0000 1000.0000 1000.0000 1000.0000 - 000000 (1 byte) - invalid
    // 1111.0000 1000.0000 1000.0001 1011.1111 - 00007F - invalid
	TEST( 4_inv_1a, -1, 0, 0,
		AR({ 0xF0, 0x80, 0x80, 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 4_inv_1b, -1, 0, 0,
		AR({ 0xF0, 0x80, 0x81, 0xBF, 'Z' }),
		AR({ BAD8, 'Z' })
	)
    // 1111.0000 1000.0000 1000.0010 1000.0000 - 000080 (2 bytes) - invalid
    // 1111.0000 1000.0000 1001.1111 1011.1111 - 0007FF - invalid
	TEST( 4_inv_2a, -1, 0, 0,
		AR({ 0xF0, 0x80, 0x82, 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 4_inv_2b, -1, 0, 0,
		AR({ 0xF0, 0x80, 0x9F, 0xBF, 'Z' }),
		AR({ BAD8, 'Z' })
	)
    // 1111.0000 1000.0000 1010.0000 1000.0000 - 000800 (3 bytes) - invalid
    // 1111.0000 1000.1111 1011.1111 1011.1111 - 00FFFF - invalid
	TEST( 4_inv_3a, -1, 0, 0,
		AR({ 0xF0, 0x80, 0xA0, 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 4_inv_3b, -1, 0, 0,
		AR({ 0xF0, 0x8F, 0xBF, 0xBF, 'Z' }),
		AR({ BAD8, 'Z' })
	)
    // 1111.0000 1001.0000 1000.0000 1000.0000 - 010000
    // 1111.0100 1000.1111 1011.1111 1011.1111 - 10FFFF
	TEST( 4_ok_1a, 0, 0, 0,
		AR({ 0xF0, 0x90, 0x80, 0x80, 'Z' }),
		AR({ 0xF0, 0x90, 0x80, 0x80, 'Z' })
	)
	TEST( 4_ok_1b, 0, 0, 0,
		AR({ 0xF4, 0x8F, 0xBF, 0xBF, 'Z' }),
		AR({ 0xF4, 0x8F, 0xBF, 0xBF, 'Z' })
	)
    // 1111.0100 1001.0000 1000.0000 1000.0000 - 110000 (4F is valid) - invalid
    // 1111.0100 1011.1111 1011.1111 1011.1111 - 13FFFF - invalid
	TEST( 4_inv_4a, -1, 0, 0,
		AR({ 0xF4, 0x90, 0x80, 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 4_inv_4b, -1, 0, 0,
		AR({ 0xF4, 0xBF, 0xBF, 0xBF, 'Z' }),
		AR({ BAD8, 'Z' })
	)
    // 1111.0101 1000.0000 1000.0000 1000.0000 - 140000 (>4F is invalid) - invalid
    // 1111.0111 1011.1111 1011.1111 1011.1111 - 1FFFFF - invalid
	TEST( 4_inv_5a, -1, 0, 0,
		AR({ 0xF5, 0x80, 0x80, 0x80, 'Z' }),
		AR({ BAD8, BAD8, BAD8, BAD8, 'Z' })
	)
	TEST( 4_inv_5b, -1, 0, 0,
		AR({ 0xF7, 0xBF, 0xBF, 0xBF, 'Z' }),
		AR({ BAD8, BAD8, BAD8, BAD8, 'Z' })
	)
    // partial
	TEST( 4_part_1, -1, 0, 0,
		AR({ 0xF0 }),
		AR({ BAD8 })
	)
	TEST( 4_part_2, -1, 0, 0,
		AR({ 0xF0, 0x90 }),
		AR({ BAD8 })
	)
	TEST( 4_part_3, -1, 0, 0,
		AR({ 0xF0, 0x90, 0x80 }),
		AR({ BAD8 })
	)
	TEST( 4_partinv_1, -1, 0, 0,
		AR({ 0xF0, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 4_partinv_2, -1, 0, 0,
		AR({ 0xF0, 0x90, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( 4_partinv_3, -1, 0, 0,
		AR({ 0xF0, 0x90, 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)

	// 5 bytes
	// 1111.10xx 10xx.xxxx 10xx.xxxx 10xx.xxxx 10xx.xxxx - (>4F is invalid) - invalid
	TEST( 5_inv_1a, -1, 0, 0,
		AR({ 0xF8, 0x80, 0x80, 0x80, 0x80, 'Z' }),
		AR({ BAD8, BAD8, BAD8, BAD8, BAD8, 'Z' })
	)
	TEST( 5_inv_1b, -1, 0, 0,
		AR({ 0xFB, 0xBF, 0xBF, 0xBF, 0xBF, 'Z' }),
		AR({ BAD8, BAD8, BAD8, BAD8, BAD8, 'Z' })
	)

	// 6 bytes
	// 1111.110x 10xx.xxxx 10xx.xxxx 10xx.xxxx 10xx.xxxx 10xx.xxxx - (>4F is invalid) - invalid
	TEST( 6_inv_1a, -1, 0, 0,
		AR({ 0xFC, 0x80, 0x80, 0x80, 0x80, 0x80, 'Z' }),
		AR({ BAD8, BAD8, BAD8, BAD8, BAD8, BAD8, 'Z' })
	)
	TEST( 6_inv_1b, -1, 0, 0,
		AR({ 0xFD, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 'Z' }),
		AR({ BAD8, BAD8, BAD8, BAD8, BAD8, BAD8, 'Z' })
	)

	// 7 bytes
	// 1111.1110 10xx.xxxx 10xx.xxxx 10xx.xxxx 10xx.xxxx 10xx.xxxx 10xx.xxxx - (>4F is invalid) - invalid
	TEST( 7_inv_1a, -1, 0, 0,
		AR({ 0xFE, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 'Z' }),
		AR({ BAD8, BAD8, BAD8, BAD8, BAD8, BAD8, BAD8, 'Z' })
	)
	TEST( 7_inv_1b, -1, 0, 0,
		AR({ 0xFE, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 0xBF, 'Z' }),
		AR({ BAD8, BAD8, BAD8, BAD8, BAD8, BAD8, BAD8, 'Z' })
	)

	//+D800 to U+DFFF
	// 1110.xxxx 10xx.xxxx 10xx.xxxx
	TEST( res_ok_1, 0, 0, 0,
		// 1110.1101 1001.1111 1011.1111 - 00D7FF
		AR({ 0xED, 0x9F, 0xBF, 'Z' }),
		AR({ 0xED, 0x9F, 0xBF, 'Z' })
	)
	TEST( res_inv_1, -1, 0, 0,
		// 1110.1101 1010.0000 1000.0000 - 00D800
		AR({ 0xED, 0xA0, 0x80, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( res_inv_2, -1, 0, 0,
		// 1110.1101 1011.1111 1011.1111 - 00DFFF
		AR({ 0xED, 0xBF, 0xBF, 'Z' }),
		AR({ BAD8, 'Z' })
	)
	TEST( res_ok_2, 0, 0, 0,
		// 1110.1110 1000.0000 1000.0000 - 00E000
		AR({ 0xEE, 0x80, 0x80, 'Z' }),
		AR({ 0xEE, 0x80, 0x80, 'Z' })
	)
