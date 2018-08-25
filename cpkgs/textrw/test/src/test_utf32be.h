
#define BAD32 C32(0x00, 0x00, 0xFF, 0xFD)

	// U+0000 to U+D7FF
	TEST( 1_ok_1a, 0, 0,
		AR({ C32(0x00, 0x00, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x00, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 1_ok_1b, 0, 0,
		AR({ C32(0x00, 0x00, 0xD7, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x00, 0xD7, 0xFF), C32(0x00, 0x00, 0x00, 'Z') })
	)

	// 0xDC00..0xDFFF - low surrogate - invalid
	TEST( 1_inv_1a, -1, 0,
		AR({ C32(0x00, 0x00, 0xDC, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 1_inv_1b, -1, 0,
		AR({ C32(0x00, 0x00, 0xDF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)

	// 0xD800..0xDBFF - high surrogate - invalid
	TEST( 2_inv_2a, -1, 0,
		AR({ C32(0x00, 0x00, 0xD8, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 2_inv_2b, -1, 0,
		AR({ C32(0x00, 0x00, 0xDB, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)

	// U+E000 to U+FFFF
	TEST( 1_ok_2a, 0, 0,
		AR({ C32(0x00, 0x00, 0xE0, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x00, 0xE0, 0x00), C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 1_ok_2b, 0, 0,
		AR({ C32(0x00, 0x00, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x00, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') })
	)

	// U+010000 to U+10FFFF
	TEST( 1_ok_3a, 0, 0,
		AR({ C32(0x00, 0x01, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x01, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 1_ok_3b, 0, 0,
		AR({ C32(0x00, 0x10, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x10, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') })
	)

	// 0x00110000..0xFFFFFFFF - invalid
	TEST( 2_inv_3a, -1, 0,
		AR({ C32(0x00, 0x11, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 2_inv_3b, -1, 0,
		AR({ C32(0xFF, 0xFF, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)
