
#define BAD32 C32(0x00, 0x00, 0xFF, 0xFD)

	TEST( 1_cut_1, -1, 0, F_IS_CUT,
		AR({ 'Z' }),
		AR({ BAD32 })
	)

	TEST( 1_cut_2, -1, 0, F_IS_CUT,
		AR({ 'Z', 'Z' }),
		AR({ BAD32 })
	)

	TEST( 1_cut_3, -1, 0, F_IS_CUT,
		AR({ 'Z', 'Z', 'Z' }),
		AR({ BAD32 })
	)

	// U+0000 to U+D7FF
	TEST( 1_ok_1a, 0, 0, 0,
		AR({ C32(0x00, 0x00, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x00, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 1_ok_1b, 0, 0, 0,
		AR({ C32(0x00, 0x00, 0xD7, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x00, 0xD7, 0xFF), C32(0x00, 0x00, 0x00, 'Z') })
	)

	// 0xDC00..0xDFFF - low surrogate - invalid
	TEST( 1_inv_1a, -1, 0, 0,
		AR({ C32(0x00, 0x00, 0xDC, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 1_inv_1b, -1, 0, 0,
		AR({ C32(0x00, 0x00, 0xDF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)

	// 0xD800..0xDBFF - high surrogate - invalid
	TEST( 2_inv_2a, -1, 0, 0,
		AR({ C32(0x00, 0x00, 0xD8, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 2_inv_2b, -1, 0, 0,
		AR({ C32(0x00, 0x00, 0xDB, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)

	// U+E000 to U+FFFF
	TEST( 1_ok_2a, 0, 0, 0,
		AR({ C32(0x00, 0x00, 0xE0, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x00, 0xE0, 0x00), C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 1_ok_2b, 0, 0, 0,
		AR({ C32(0x00, 0x00, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x00, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') })
	)

	// U+010000 to U+10FFFF
	TEST( 1_ok_3a, 0, 0, 0,
		AR({ C32(0x00, 0x01, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x01, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 1_ok_3b, 0, 0, 0,
		AR({ C32(0x00, 0x10, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ C32(0x00, 0x10, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') })
	)

	// 0x00110000..0xFFFFFFFF - invalid
	TEST( 2_inv_3a, -1, 0, 0,
		AR({ C32(0x00, 0x11, 0x00, 0x00), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)
	TEST( 2_inv_3b, -1, 0, 0,
		AR({ C32(0xFF, 0xFF, 0xFF, 0xFF), C32(0x00, 0x00, 0x00, 'Z') }),
		AR({ BAD32, C32(0x00, 0x00, 0x00, 'Z') })
	)

	// BOM 8
   	TEST( 32_bom_8_ok, 0, SQTEXTENC_UTF8, F_IS_BOM,
		AR({ 0xEF, 0xBB, 0xBF, 'Z' }),
		AR({ C32(0x00, 0x00, 0x00, 'Z') })
	)
   	TEST( 32_bom_8_eof, 0, SQTEXTENC_UTF8, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xEF, 0xBB, 0xBF }),
		AR({ '!' })
	)
   	TEST( 32_bom_8_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xEF }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_8_not_1a, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xEF, 'Z' }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_8_not_2, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xEF, 0xBB }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_8_not_2a, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xEF, 0xBB, 'Z' }),
		AR({ BAD32 })
	)
	TEST( 32_bom_8_inv_1a, -1, SQTEXTENC_UTF8, F_IS_BOM,
		AR({ 0xEF, 0xBB, 0xBF, 0xEF }),
		AR({ BAD32 })
	)
//	TEST( 32_bom_8_inv_1b, -1, SQTEXTENC_UTF8, F_IS_BOM,
//		AR({ 0xEF, 0xBB, 0xBF, 0xEF, 0x80, 0x80, 0x80, 'Z' }),
//		AR({ C32(0x00, 0x00, 0xF0, 0x00), BAD32, C32(0x00, 0x00, 0x00, 'Z') })
//	)

	// BOM 16be
   	TEST( 32_bom_16be_ok, 0, SQTEXTENC_UTF16BE, F_IS_BOM,
		AR({ 0xFE, 0xFF, 0x00, 'Z' }),
		AR({ C32(0x00, 0x00, 0x00, 'Z') })
	)
   	TEST( 32_bom_16be_eof, 0, SQTEXTENC_UTF16BE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xFE, 0xFF}),
		AR({ '!' })
	)
   	TEST( 32_bom_16be_part, 0, SQTEXTENC_UTF16BE, F_IS_BOM | F_IS_CUT,
		AR({ 0xFE, 0xFF, 0x00 }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_16be_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xFE }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_16be_not_1a, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xFE, 'Z' }),
		AR({ BAD32 })
	)

	// BOM 16le
   	TEST( 32_bom_16le_ok, 0, SQTEXTENC_UTF16LE, F_IS_BOM,
		AR({ 0xFF, 0xFE, 'Z', 0x00 }),
		AR({ C32(0x00, 0x00, 0x00, 'Z') })
	)
   	TEST( 32_bom_16le_eof, 0, SQTEXTENC_UTF16LE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xFF, 0xFE}),
		AR({ '!' })
	)
   	TEST( 32_bom_16le_part, 0, SQTEXTENC_UTF16LE, F_IS_BOM | F_IS_CUT,
		AR({ 0xFF, 0xFE, 'Z' }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_16le_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xFF }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_16le_not_1a, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xFF, 'Z' }),
		AR({ BAD32 })
	)

	// BOM 32be
   	TEST( 32_bom_32be_ok, 0, SQTEXTENC_UTF32BE, F_IS_BOM,
		AR({ 0x00, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x00, 'Z' }),
		AR({ C32(0x00, 0x00, 0x00, 'Z') })
	)
   	TEST( 32_bom_32be_eof, 0, SQTEXTENC_UTF32BE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0x00, 0x00, 0xFE, 0xFF }),
		AR({ '!' })
	)
   	TEST( 32_bom_32be_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0x00 }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_32be_not_1a, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0x00, 'Z' }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_32be_not_2, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0x00, 0x00 }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_32be_not_2a, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0x00, 0x00, 'Z' }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_32be_not_3, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0x00, 0x00, 0xFE }),
		AR({ BAD32 })
	)

	// BOM 32le
   	TEST( 32_bom_32le_ok, 0, SQTEXTENC_UTF32LE, F_IS_BOM,
		AR({ 0xFF, 0xFE, 0x00, 0x00, 'Z', 0x00, 0x00, 0x00 }),
		AR({ C32(0x00, 0x00, 0x00, 'Z') })
	)
   	TEST( 32_bom_32le_eof, 0, SQTEXTENC_UTF32LE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xFF, 0xFE, 0x00, 0x00 }),
		AR({ '!' })
	)
   	TEST( 32_bom_32le_not_1, 0, -1, F_IS_BOM | F_IS_CUT,
		AR({ 0xFF }),
		AR({ BAD32 })
	)
   	TEST( 32_bom_32le_not_2, 0, SQTEXTENC_UTF16LE, F_IS_BOM | F_RES_EMPTY,
		AR({ 0xFF, 0xFE }),
		AR({ '!' })
	)
   	TEST( 32_bom_32le_not_3, 0, SQTEXTENC_UTF16LE, F_IS_BOM | F_IS_CUT,
		AR({ 0xFF, 0xFE, 0x00 }),
		AR({ BAD32 })
	)
