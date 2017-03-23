
#include <squirrel.h>
#include <sqtool.h>
#include <sqstreamreader.h>

static const uint8_t utf8_lengths[16] =
{
	1,1,1,1,1,1,1,1,        /* 0000 to 0111 : 1 byte (plain ASCII) */
	0,0,0,0,                /* 1000 to 1011 : not valid */
	2,2,                    /* 1100 to 1101 : 2 bytes */
	3,                      /* 1110 : 3 bytes */
	4                       /* 1111 :4 bytes */
};

static unsigned char utf8_byte_masks[5] = {0,0,0x1f,0x0f,0x07};

struct TextConverter
{
	virtual SQInteger BytesRead( uint8_t *p, SQInteger n) = 0;
	virtual SQInteger BytesWrite( uint8_t *p, SQInteger n) = 0;
	
	SQInteger (*BytesReadChar)( uint32_t *pwc);
	SQInteger (*BytesWriteChar)( uint32_t wc);

	uint32_t _bad_char = '?';

	/* ================
		Read
	================ */

	SQInteger BytesReadU8( uint8_t *pc)
	{
		return BytesRead( pc, 1);
	}
	
	SQInteger BytesReadU16LE( uint16_t *pc)
	{
		if( BytesRead( pc, 2) == 2) {
			*pc = (buf[1] << 8) | buf[0];
			return SQ_OK;
		}
		return SQ_ERROR;
	}
	
	SQInteger BytesReadU16BE( uint16_t *pc)
	{
		uint8_t buf[2];
		if( BytesRead( buf, 2) == 2) {
			*pc = (buf[0] << 8) | buf[1];
			return SQ_OK;
		}
		return SQ_ERROR;
	}

	SQBool isRdBigEndian = SQTrue;	// Default is Big Endian
	
	SQInteger BytesReadU16( uint16_t *pc) { if( !isRdBigEndian) return BytesReadU16LE(pc); else return BytesReadU16BE(pc); };
	
	/* ================
		Write
	================ */

	SQInteger BytesWriteU8( uint8_t c)
	{
		return BytesWrite( &c, 1);
	}
	
	SQInteger BytesWriteU16LE( uint16_t c)
	{
		uint8_t buf[2] = { (uint8_t)c, (uint8_t)(c>>8) };
		if( BytesWrite( buf, 2) == 2) {
			return SQ_OK;
		}
		return SQ_ERROR;
	}
	
	SQInteger BytesWriteU16BE( uint16_t c)
	{
		uint8_t buf[2] = { (uint8_t)(c>>8), (uint8_t)c };
		if( BytesWrite( buf, 2) == 2) {
			return SQ_OK;
		}
		return SQ_ERROR;
	}

	SQBool isWrBigEndian = SQTrue;	// Default is Big Endian
	
	SQInteger BytesWriteU16( uint16_t c) { if( !isWrBigEndian) return BytesWriteU16LE(c); else return BytesWriteU16BE(c); };
	
	/* ================
		Raw
	================ */
	
	SQInteger Read_RAW( uint32_t *pwc)
	{
		uint8_t c;
		if( SQ_FAILED(BytesReadU8( &c))) return -1;		// EOF (SQCCV_TOOFEW)
		*pwc = c;
		return SQ_OK;
	}
	
	SQInteger Write_RAW( uint32_t wc)
	{
		uint8_t c = (uint8_t)wc;
		return BytesWriteU8( c);
	}
	
	/* ================
		ASCII
	================ */
	
	SQInteger Read_ASCII( uint32_t *pwc)
	{
		uint8_t c;
		if( BytesReadU8( &c) != 0) return -1;		// EOF (SQCCV_TOOFEW)
		*pwc = (c <= 0x7F) ? c : _bad_char;
		return SQ_OK;
	}
	
	SQInteger Write_ASCII( uint32_t wc)
	{
		uint8_t c = (wc <= 0x7F) ? (uint8_t)wc : (uint8_t)_bad_char;
		return BytesWriteU8( c);
	}
	
	/* ================
		UTF-8
	================ */

	SQInteger Read_UTF8( uint32_t *pwc)
	{
		uint8_t c;
		if( BytesReadU8( &c) != 0) return -1;		// EOF (SQCCV_TOOFEW)
		if( c < 0x80) { *pwc = c; return SQ_OK; }
		else if( c < 0xC2) { *pwc = _bad_char; return 1; }	// SQCCV_ILSEQ
		else {
			SQInteger codelen = utf8_lengths[ c >> 4];
			uint32_t wc = c & utf8_byte_masks[codelen];
			while( --codelen) {
				if( BytesReadU8( &c) != 0) { *pwc = _bad_char; return -1; }		// EOF (SQCCV_TOOFEW)
				if( (c & 0xC0) != 0x80) { *pwc = _bad_char; return 1; }	// SQCCV_ILSEQ
				wc <<= 6; wc |= c & 0x3F;
			}
			*pwc = wc;
			return SQ_OK;
		}
	}

	SQInteger Write_UTF8( uint32_t wc)
	{
		if( wc < 0xD800 || (wc >= 0xE000 && wc != 0xFFFE && wc < 0x110000)) {
			SQInteger codelen;

			if( wc <= 0x007F)			{ uint8_t c = (uint8_t)wc; return BytesWrite( &c, 1); }
			else if( wc <= 0x07FF)		{ codelen = 2; }
			else if( wc <= 0xFFFFL)		{ codelen = 3; }
			else						{ codelen = 4; }
		
			uint8_t r[4];
			switch( codelen)
			{ /* note: code falls through cases! */
				case 4: r[3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
				case 3: r[2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
				case 2: r[1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
				case 1: r[0] = wc;
			}
			return BytesWrite( r, codelen);
		}
		return Write_UTF16(_bad_char);	// SQCCV_ILUNI
	}
	
	/* ================
		UTF-16
	================ */

	SQInteger Read_UTF16( uint32_t *pwc)
	{
		while(1) {
			uint16_t c;
			if( BytesReadU16( &c) != 0) return -1;	// EOF (SQCCV_TOOFEW)
			if( c == 0xFEFF)		{ /* ok */ }
			else if( c == 0xFFFE)  { isRdBigEndian = !isRdBigEndian; }
			else if( c >= 0xD800 && c < 0xDC00) {
				uint32_t wc = c;
				if( BytesReadU16( &c) != 0) return _bad_char;	// EOF (SQCCV_TOOFEW)
				if( c >= 0xDC00 && c < 0xE000) {
					wc = 0x10000 + ((wc - 0xD800) << 10) + (c - 0xDC00);
					return wc;
				}
				else return _bad_char;	// SQCCV_ILSEQ
			}
			else if( c >= 0xDC00 && c < 0xE000) { *pwc = _bad_char; return 1; }		// SQCCV_ILSEQ
			else { *pwc = c; return SQ_OK; }
		}
	}

	SQInteger Write_UTF16( uint32_t wc)
	{
		if( wc < 0xD800 || (wc >= 0xE000 && wc != 0xFFFE && wc < 0x110000)) {
			// check and write BOM
			if( wc >= 0x10000) {
				uint16_t w1, w2;

				wc -= 0x10000;
				w2 = 0xDC00 | (wc & 0x03FF);
				w1 = 0xD800 | ((wc >> 10) & 0x03FF);
				
				if( BytesWriteU16( w1) == 0)
					if( BytesWriteU16( w2) == 0)
						return SQ_OK;
				return SQ_ERROR;
			}
			else if( wc < 0x10000 && n >= 2) {
				return BytesWriteU16( (uint16_t)wc);
			}
		}
		else return Write_UTF16(_bad_char); // SQCCV_ILUNI;
	}

};

#define CBUFF_SIZE	8

struct SQTTextReader : public TextConverter
{
	SQTTextReader( SQStream *stream, sqBool owns) { _stream = stream; _owns = owns };
	SQTTextReader( SQStream *stream, const SQString *encoding, SQBool guess, sqBool owns) { _stream = stream };
	
	SQInteger BytesRead( uint8_t *p, SQInteger n)
	{
		if( _stream->Read( pc, n) == n)
			return SQ_OK;
		return SQ_ERROR;
	}
	
	SQInteger BytesWrite( uint8_t *p, SQInteger n)
	{
		if( (_cbuf_len + n) > CBUFF_SIZE) return SQ_ERROR;
		memcpy( _cbuf + _cbuf_len, p, n);
		_cbuf_len += n;
		return SQ_OK;
	}
	
	SQInteger ReadChar( SQChar *pc)
	{
		if( _cbuf_pos == _cbuf_len) {
			uint32_t wc;
			_cbuf_pos = 0;
			_cbuf_len = 0;
			if( BytesReadChar( &wc) == 0) {
				BytesWriteChar( wc);
			}
		}
		if( _cbuf_pos == _cbuf_len) return SQ_ERROR;
		*pc = _cbuff[_cbuf_pos++];
		return SQ_OK;
	}
	
	void Close() {
		if( _owns && _stream) {
			_stream->Close();
		}
		_stream = NULL;
		_owns = SQFlase;
	}
	
	SQStream *_stream;
	SQBool _owns;
	
	SQChar _cbuff[CBUFF_SIZE];
	SQInteger _cbuf_pos;
	SQInteger _cbuf_len;
};

