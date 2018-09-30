
#define BAD16 C16(0xFF, 0xFD)

	// 1 bytes

	TEST( 1_cut_1, -1, 0, F_IS_CUT,
		AR({ 'Z' }),
		AR({ BAD16 })
	)

	// 2 bytes

	// U+0000 to U+D7FF
	TEST( 2_ok_1a, 0, 0, 0,
		AR({ C16(0x00, 0x00), C16(0x00, 'Z') }),
		AR({ C16(0x00, 0x00), C16(0x00, 'Z') })
	)
	TEST( 2_ok_1b, 0, 0, 0,
		AR({ C16(0xD7, 0xFF), C16(0x00, 'Z') }),
		AR({ C16(0xD7, 0xFF), C16(0x00, 'Z') })
	)

	// 0xDC00..0xDFFF - low surrogate - invalid
	TEST( 2_inv_1a, -1, 0, 0,
		AR({ C16(0xDC, 0x00), C16(0x00, 'Z') }),
		AR({ BAD16, C16(0x00, 'Z') })
	)
	TEST( 2_inv_1b, -1, 0, 0,
		AR({ C16(0xDF, 0xFF), C16(0x00, 'Z') }),
		AR({ BAD16, C16(0x00, 'Z') })
	)

	// U+E000 to U+FFFF
	TEST( 2_ok_2a, 0, 0, 0,
		AR({ C16(0xE0, 0x00), C16(0x00, 'Z') }),
		AR({ C16(0xE0, 0x00), C16(0x00, 'Z') })
	)
	TEST( 2_ok_2b, 0, 0, 0,
		AR({ C16(0xFF, 0xFF), C16(0x00, 'Z') }),
		AR({ C16(0xFF, 0xFF), C16(0x00, 'Z') })
	)

	// 3 bytes

	TEST( 3_cut_1, -1, 0, F_IS_CUT,
		AR({ C16(0xD8, 0x00), 'Z' }),
		AR({ BAD16 })
	)

	// 4 bytes

	// 0xD800..0xDBFF, 0xDC00..0xDFFF
	TEST( 4_ok_1a, 0, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0xDC, 0x00), C16(0x00, 'Z') }),
		AR({ C16(0xD8, 0x00), C16(0xDC, 0x00), C16(0x00, 'Z') })
	)
	TEST( 4_ok_1b, 0, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0xDF, 0xFF), C16(0x00, 'Z') }),
		AR({ C16(0xD8, 0x00), C16(0xDF, 0xFF), C16(0x00, 'Z') })
	)
	TEST( 4_ok_1c, 0, 0, 0,
		AR({ C16(0xDB, 0xFF), C16(0xDC, 0x00), C16(0x00, 'Z') }),
		AR({ C16(0xDB, 0xFF), C16(0xDC, 0x00), C16(0x00, 'Z') })
	)
	TEST( 4_ok_1d, 0, 0, 0,
		AR({ C16(0xDB, 0xFF), C16(0xDF, 0xFF), C16(0x00, 'Z') }),
		AR({ C16(0xDB, 0xFF), C16(0xDF, 0xFF), C16(0x00, 'Z') })
	)

	// 0xD800..0xDBFF - high surrogate at end - invalid
	TEST( 4_part_1a, -1, 0, 0,
		AR({ C16(0xD8, 0x00) }),
		AR({ BAD16 })
	)
	TEST( 4_part_1b, -1, 0, 0,
		AR({ C16(0xDB, 0xFF) }),
		AR({ BAD16 })
	)

	// 0xD800..0xDBFF, 0x0000..0xD7FF - high surrogate followed by code point - invalid seq
	TEST( 4_partinv_1a, -1, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0x00, 'Z') }),
		AR({ BAD16, C16(0x00, 'Z') })
	)
	TEST( 4_partinv_1b, -1, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0xD7, 0xFF) }),
		AR({ BAD16, C16(0xD7, 0xFF) })
	)
	// 0xD800..0xDBFF, 0xD800..0xDBFF - high surrogate followed by high surrogate - invalid seq
	TEST( 4_partinv_1c, -1, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0xD8, 0x00) }),
		AR({ BAD16, BAD16 })
	)
	TEST( 4_partinv_1d, -1, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0xDB, 0xFF) }),
		AR({ BAD16, BAD16 })
	)
	// 0xD800..0xDBFF, 0xE000..0xFFFF - high surrogate followed by code point 0xE000..0xFFFF - invalid seq
	TEST( 4_partinv_1e, -1, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0xE0, 0x00) }),
		AR({ BAD16, C16(0xE0, 0x00) })
	)
	TEST( 4_partinv_1f, -1, 0, 0,
		AR({ C16(0xD8, 0x00), C16(0xFF, 0xFF) }),
		AR({ BAD16, C16(0xFF, 0xFF) })
	)

	// BOM 8
   	TEST( 16_bom_8_ok, 0, SQTEXTENC_UTF8, F_IS_BOM,
		AR({ 0xEF, 0xBB, 0xBF, 'Z' }),
		AR({ C16(0x00, 'Z') })
	)
   	TEST( 16_bom_8_eof, 0, SQTEXTENC_UTF8, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xEF, 0xBB, 0xBF }),
		AR({ '!' })
	)
   	TEST( 16_bom_8_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xEF }),
		AR({ BAD16 })
	)
   	TEST( 16_bom_8_not_1a, 0, -1, F_IS_BOM,
		AR({ 0xEF, 'Z' }),
		AR({ 0xEF, 'Z' })
	)
   	TEST( 16_bom_8_not_2, 0, -1, F_IS_BOM,
		AR({ 0xEF, 0xBB }),
		AR({ 0xEF, 0xBB })
	)
   	TEST( 16_bom_8_not_2a, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xEF, 0xBB, 'Z' }),
		AR({ 0xEF, 0xBB, BAD16 })
	)
	TEST( 16_bom_8_inv_1a, -1, SQTEXTENC_UTF8, F_IS_BOM,
		AR({ 0xEF, 0xBB, 0xBF, 0xEF }),
		AR({ BAD16 })
	)
//	TEST( 16_bom_8_inv_1b, -1, SQTEXTENC_UTF8, F_IS_BOM,
//		AR({ 0xEF, 0xBB, 0xBF, 0xEF, 0x80, 0x80, 0x80, 'Z' }),
//		AR({ C16(0xF0, 0x00), BAD16, C16(0x00, 'Z') })
//	)

	// BOM 16be
   	TEST( 16_bom_16be_ok, 0, SQTEXTENC_UTF16BE, F_IS_BOM,
		AR({ 0xFE, 0xFF, 0x00, 'Z' }),
		AR({ C16(0x00, 'Z') })
	)
   	TEST( 16_bom_16be_eof, 0, SQTEXTENC_UTF16BE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xFE, 0xFF}),
		AR({ '!' })
	)
   	TEST( 16_bom_16be_part, 0, SQTEXTENC_UTF16BE, F_IS_BOM | F_IS_CUT,
		AR({ 0xFE, 0xFF, 0x00 }),
		AR({ BAD16 })
	)
   	TEST( 16_bom_16be_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xFE }),
		AR({ BAD16 })
	)
   	TEST( 16_bom_16be_not_1a, 0, -1, F_IS_BOM,
		AR({ 0xFE, 'Z' }),
		AR({ 0xFE, 'Z' })
	)

	// BOM 16le
   	TEST( 16_bom_16le_ok, 0, SQTEXTENC_UTF16LE, F_IS_BOM,
		AR({ 0xFF, 0xFE, 'Z', 0x00 }),
		AR({ C16(0x00, 'Z') })
	)
   	TEST( 16_bom_16le_eof, 0, SQTEXTENC_UTF16LE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xFF, 0xFE}),
		AR({ '!' })
	)
   	TEST( 16_bom_16le_part, 0, SQTEXTENC_UTF16LE, F_IS_BOM | F_IS_CUT,
		AR({ 0xFF, 0xFE, 'Z' }),
		AR({ BAD16 })
	)
   	TEST( 16_bom_16le_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xFF }),
		AR({ BAD16 })
	)
   	TEST( 16_bom_16le_not_1a, 0, -1, F_IS_BOM,
		AR({ 0xFF, 'Z' }),
		AR({ 0xFF, 'Z' })
	)

	// BOM 32be
   	TEST( 16_bom_32be_ok, 0, SQTEXTENC_UTF32BE, F_IS_BOM,
		AR({ 0x00, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x00, 'Z' }),
		AR({ C16(0x00, 'Z') })
	)
   	TEST( 16_bom_32be_eof, 0, SQTEXTENC_UTF32BE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0x00, 0x00, 0xFE, 0xFF }),
		AR({ '!' })
	)
   	TEST( 16_bom_32be_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0x00 }),
		AR({ BAD16 })
	)
   	TEST( 16_bom_32be_not_1a, 0, -1, F_IS_BOM,
		AR({ 0x00, 'Z' }),
		AR({ 0x00, 'Z' })
	)
   	TEST( 16_bom_32be_not_2, 0, -1, F_IS_BOM,
		AR({ 0x00, 0x00 }),
		AR({ 0x00, 0x00 })
	)
   	TEST( 16_bom_32be_not_2a, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0x00, 0x00, 'Z' }),
		AR({ 0x00, 0x00, BAD16 })
	)
   	TEST( 16_bom_32be_not_3, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0x00, 0x00, 0xFE }),
		AR({ 0x00, 0x00, BAD16 })
	)
   	TEST( 16_bom_32be_not_3a, 0, -1, F_IS_BOM,
		AR({ 0x00, 0x00, 0xFE, 'Z' }),
		AR({ 0x00, 0x00, 0xFE, 'Z' })
	)

	// BOM 32le
   	TEST( 16_bom_32le_ok, 0, SQTEXTENC_UTF32LE, F_IS_BOM,
		AR({ 0xFF, 0xFE, 0x00, 0x00, 'Z', 0x00, 0x00, 0x00 }),
		AR({ C16(0x00, 'Z') })
	)
   	TEST( 16_bom_32le_eof, 0, SQTEXTENC_UTF32LE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xFF, 0xFE, 0x00, 0x00 }),
		AR({ '!' })
	)
   	TEST( 16_bom_32le_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xFF }),
		AR({ BAD16 })
	)
   	TEST( 16_bom_32le_not_2, 0, SQTEXTENC_UTF16LE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xFF, 0xFE }),
		AR({ '!' })
	)
   	TEST( 16_bom_32le_not_3, 0, SQTEXTENC_UTF16LE, F_IS_BOM | F_IS_CUT,
		AR({ 0xFF, 0xFE, 0x00 }),
		AR({ BAD16 })
	)
