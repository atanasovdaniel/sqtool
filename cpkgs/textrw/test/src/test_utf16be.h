
#define BAD16 C16(0xFF, 0xFD)

	// 2 bytes

	// U+0000 to U+D7FF
	TEST( 1_ok_1a, 0, 0,
		AR({ C16(0x00, 0x00), C16(0x00, 'Z') }),
		AR({ C16(0x00, 0x00), C16(0x00, 'Z') })
	)
	TEST( 1_ok_1b, 0, 0,
		AR({ C16(0xD7, 0xFF), C16(0x00, 'Z') }),
		AR({ C16(0xD7, 0xFF), C16(0x00, 'Z') })
	)

	// 0xDC00..0xDFFF - low surrogate - invalid
	TEST( 1_inv_1a, -1, 0,
		AR({ C16(0xDC, 0x00), C16(0x00, 'Z') }),
		AR({ BAD16, C16(0x00, 'Z') })
	)
	TEST( 1_inv_1b, -1, 0,
		AR({ C16(0xDF, 0xFF), C16(0x00, 'Z') }),
		AR({ BAD16, C16(0x00, 'Z') })
	)

	// U+E000 to U+FFFF
	TEST( 1_ok_2a, 0, 0,
		AR({ C16(0xE0, 0x00), C16(0x00, 'Z') }),
		AR({ C16(0xE0, 0x00), C16(0x00, 'Z') })
	)
	TEST( 1_ok_2b, 0, 0,
		AR({ C16(0xFF, 0xFF), C16(0x00, 'Z') }),
		AR({ C16(0xFF, 0xFF), C16(0x00, 'Z') })
	)

	// 4 bytes

	// 0xD800..0xDBFF, 0xDC00..0xDFFF
	TEST( 2_ok_1a, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0xDC, 0x00), C16(0x00, 'Z') }),
		AR({ C16(0xD8, 0x00), C16(0xDC, 0x00), C16(0x00, 'Z') })
	)
	TEST( 2_ok_1b, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0xDF, 0xFF), C16(0x00, 'Z') }),
		AR({ C16(0xD8, 0x00), C16(0xDF, 0xFF), C16(0x00, 'Z') })
	)
	TEST( 2_ok_1c, 0, 0,
		AR({ C16(0xDB, 0xFF), C16(0xDC, 0x00), C16(0x00, 'Z') }),
		AR({ C16(0xDB, 0xFF), C16(0xDC, 0x00), C16(0x00, 'Z') })
	)
	TEST( 2_ok_1d, 0, 0,
		AR({ C16(0xDB, 0xFF), C16(0xDF, 0xFF), C16(0x00, 'Z') }),
		AR({ C16(0xDB, 0xFF), C16(0xDF, 0xFF), C16(0x00, 'Z') })
	)

	// 0xD800..0xDBFF - high surrogate at end - invalid
	TEST( 2_part_1a, -1, 0,
		AR({ C16(0xD8, 0x00) }),
		AR({ BAD16 })
	)
	TEST( 2_part_1b, -1, 0,
		AR({ C16(0xDB, 0xFF) }),
		AR({ BAD16 })
	)

	// 0xD800..0xDBFF, 0x0000..0xD7FF - high surrogate followed by code point - invalid seq
	TEST( 2_partinv_1a, -1, 0,
		AR({ C16(0xD8, 0x00), C16(0x00, 'Z') }),
		AR({ BAD16, C16(0x00, 'Z') })
	)
	TEST( 2_partinv_1b, -1, 0,
		AR({ C16(0xD8, 0x00), C16(0xD7, 0xFF) }),
		AR({ BAD16, C16(0xD7, 0xFF) })
	)
	// 0xD800..0xDBFF, 0xD800..0xDBFF - high surrogate followed by high surrogate - invalid seq
	TEST( 2_partinv_1c, -1, 0,
		AR({ C16(0xD8, 0x00), C16(0xD8, 0x00) }),
		AR({ BAD16, BAD16 })
	)
	TEST( 2_partinv_1d, -1, 0,
		AR({ C16(0xD8, 0x00), C16(0xDB, 0xFF) }),
		AR({ BAD16, BAD16 })
	)
	// 0xD800..0xDBFF, 0xE000..0xFFFF - high surrogate followed by code point 0xE000..0xFFFF - invalid seq
	TEST( 2_partinv_1e, -1, 0,
		AR({ C16(0xD8, 0x00), C16(0xE0, 0x00) }),
		AR({ BAD16, C16(0xE0, 0x00) })
	)
	TEST( 2_partinv_1f, -1, 0,
		AR({ C16(0xD8, 0x00), C16(0xFF, 0xFF) }),
		AR({ BAD16, C16(0xFF, 0xFF) })
	)