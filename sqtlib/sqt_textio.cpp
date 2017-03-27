#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <squirrel.h>
#include <sqtool.h>
#include <sqstdio.h>
#include <sqtstreamreader.h>

#define SQTC_ILSEQ	(1)
#define SQTC_ILUNI	(2)

typedef SQInteger (*SQTCReadFct)( SQUserPointer user, uint8_t *p, SQInteger n);
typedef SQInteger (*SQTCWriteFct)( SQUserPointer user, const uint8_t *p, SQInteger n);

static const uint8_t utf8_lengths[16] =
{
	1,1,1,1,1,1,1,1,        /* 0000 to 0111 : 1 byte (plain ASCII) */
	0,0,0,0,                /* 1000 to 1011 : not valid */
	2,2,                    /* 1100 to 1101 : 2 bytes */
	3,                      /* 1110 : 3 bytes */
	4                       /* 1111 :4 bytes */
};

static unsigned char utf8_byte_masks[5] = {0,0,0x1f,0x0f,0x07};

struct TextConverter;

typedef struct encodings_list_tag
{
	const SQChar *name;
	SQInteger (TextConverter::*Read)( uint32_t *pwc);
	SQInteger (TextConverter::*Write)( uint32_t wc);

} encodings_list_t;

static const encodings_list_tag *find_encoding( const SQChar *name);

struct TextConverter
{
	TextConverter( SQTCReadFct read, SQTCWriteFct write, SQUserPointer user) {
		BytesRead = read;
		BytesWrite = write;
		_BytesUser = user;
		_BytesReadChar = &TextConverter::Read_UTF8;
		_BytesWriteChar = &TextConverter::Write_UTF8;
	}
	
	SQTCReadFct BytesRead;
	SQTCWriteFct BytesWrite;
	SQUserPointer _BytesUser;
	
	uint32_t _bad_char = '?';

	SQInteger (TextConverter::*_BytesReadChar)( uint32_t *pwc);
	SQInteger (TextConverter::*_BytesWriteChar)( uint32_t wc);

	SQInteger ReadChar( uint32_t *pwc) { return (this->*_BytesReadChar)( pwc); }
	SQInteger WriteChar( uint32_t wc) { return (this->*_BytesWriteChar)( wc); }

	/* ================
		Read
	================ */

	SQInteger BytesReadU8( uint8_t *pc)
	{
		return (BytesRead( _BytesUser, pc, 1) == 1) ? SQ_OK : SQ_ERROR;
	}
	
	SQInteger BytesReadU16LE( uint16_t *pc)
	{
		uint8_t buf[2];
		if( BytesRead( _BytesUser, buf, 2) == 2) {
			*pc = (buf[1] << 8) | buf[0];
			return SQ_OK;
		}
		return SQ_ERROR;
	}
	
	SQInteger BytesReadU16BE( uint16_t *pc)
	{
		uint8_t buf[2];
		if( BytesRead( _BytesUser, buf, 2) == 2) {
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
		return (BytesWrite( _BytesUser, &c, 1) == 1) ? SQ_OK : SQ_ERROR;;
	}
	
	SQInteger BytesWriteU16LE( uint16_t c)
	{
		uint8_t buf[2] = { (uint8_t)c, (uint8_t)(c>>8) };
		if( BytesWrite( _BytesUser, buf, 2) == 2) {
			return SQ_OK;
		}
		return SQ_ERROR;
	}
	
	SQInteger BytesWriteU16BE( uint16_t c)
	{
		uint8_t buf[2] = { (uint8_t)(c>>8), (uint8_t)c };
		if( BytesWrite( _BytesUser, buf, 2) == 2) {
			return SQ_OK;
		}
		return SQ_ERROR;
	}

	SQBool isWrBigEndian = SQTrue;	// Default is Big Endian
	
	SQInteger BytesWriteU16( uint16_t c) { if( !isWrBigEndian) return BytesWriteU16LE(c); else return BytesWriteU16BE(c); };
	
	/* ================
		"" - native char
	================ */
	
	SQInteger Read_CHAR( uint32_t *pwc)
	{
		SQChar c;
		if( BytesRead( _BytesUser, (uint8_t*)&c, sizeof(SQChar)) != sizeof(SQChar)) return SQ_ERROR;
		*pwc = c;
		return SQ_OK;
	}
	
	SQInteger Write_CHAR( uint32_t wc)
	{
		SQChar c = (SQChar)wc;
		return (BytesWrite( _BytesUser, (uint8_t*)&c, sizeof(SQChar)) == sizeof(SQChar)) ? SQ_OK : SQ_ERROR;
	}
	
	/* ================
		ASCII
	================ */
	
	SQInteger Read_ASCII( uint32_t *pwc)
	{
		uint8_t c;
		if( SQ_FAILED(BytesReadU8( &c))) return SQ_ERROR;		// EOF (SQCCV_TOOFEW)
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
		if( SQ_FAILED(BytesReadU8( &c))) return SQ_ERROR;		// EOF (SQCCV_TOOFEW)
		if( c < 0x80) { *pwc = c; return SQ_OK; }
		else if( c < 0xC2) { *pwc = _bad_char; return SQTC_ILSEQ; }	// SQCCV_ILSEQ
		else {
			SQInteger codelen = utf8_lengths[ c >> 4];
			uint32_t wc = c & utf8_byte_masks[codelen];
			while( --codelen) {
				if( SQ_FAILED(BytesReadU8( &c))) { return SQ_ERROR; }		// EOF (SQCCV_TOOFEW)
				if( (c & 0xC0) != 0x80) { *pwc = _bad_char; return SQTC_ILSEQ; }	// SQCCV_ILSEQ
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

			if( wc <= 0x007F)			{ uint8_t c = (uint8_t)wc; return BytesWrite( _BytesUser, &c, 1); }
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
			return BytesWrite( _BytesUser, r, codelen);
		}
		Write_UTF8(_bad_char);	// SQCCV_ILUNI
		return SQTC_ILUNI;
	}
	
	/* ================
		UTF-16
	================ */

	SQInteger Read_UTF16( uint32_t *pwc)
	{
		while(1) {
			uint16_t c;
			if( SQ_FAILED(BytesReadU16( &c))) return SQ_ERROR;	// EOF (SQCCV_TOOFEW)
			if( c == 0xFEFF)		{ /* ok */ }
			else if( c == 0xFFFE)  { isRdBigEndian = !isRdBigEndian; }
			else if( c >= 0xD800 && c < 0xDC00) {
				uint32_t wc = c;
				if( SQ_FAILED(BytesReadU16( &c))) return SQ_ERROR;	// EOF (SQCCV_TOOFEW)
				if( c >= 0xDC00 && c < 0xE000) {
					wc = 0x10000 + ((wc - 0xD800) << 10) + (c - 0xDC00);
					return wc;
				}
				else return _bad_char;	// SQCCV_ILSEQ
			}
			else if( c >= 0xDC00 && c < 0xE000) { *pwc = _bad_char; return SQTC_ILSEQ; }		// SQCCV_ILSEQ
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
				
				if( SQ_SUCCEEDED(BytesWriteU16( w1)))
					if( SQ_SUCCEEDED(BytesWriteU16( w2)))
						return SQ_OK;
				return SQ_ERROR;
			}
			else if( wc < 0x10000) {
				return BytesWriteU16( (uint16_t)wc);
			}
		}
		Write_UTF16(_bad_char); // SQCCV_ILUNI;
		return SQTC_ILUNI;
	}

	SQInteger SetReadEncoding( const SQChar *name)
	{
		const encodings_list_tag *enc = find_encoding( name);
		if( enc != NULL) {
			_BytesReadChar = enc->Read;
			return SQ_OK;
		}
		//_BytesReadChar = &TextConverter::Read_NULL;
		return SQ_ERROR;
	}

	SQInteger SetWriteEncoding( const SQChar *name)
	{
		const encodings_list_tag *enc = find_encoding( name);
		if( enc != NULL) {
			_BytesWriteChar = enc->Write;
			return SQ_OK;
		}
		//_BytesWriteChar = &TextConverter::Write_NULL;
		return SQ_ERROR;
	}
};

/* ====================================
		conversion names
==================================== */

static encodings_list_t encodings_list[] = {
	{ _SC(""),			&TextConverter::Read_CHAR, &TextConverter::Write_CHAR },
	{ _SC("ASCII"),		&TextConverter::Read_ASCII, &TextConverter::Write_ASCII },
	{ _SC("UTF-8"),		&TextConverter::Read_UTF8,  &TextConverter::Write_UTF8  },
	{ _SC("UTF-16"),	&TextConverter::Read_UTF16, &TextConverter::Write_UTF16 },
	{ NULL, NULL, NULL }
};

static int compare_encoding_name( const SQChar *iname, const SQChar *oname)
{
	SQChar i, o;
	while( ((i = *iname)!=_SC('\0')) && ((o = *oname)!=_SC('\0'))) {
		if( !(i==o) && !(i>=_SC('A') && (i^o)==0x20)
		  && !( i==_SC('-') && o==_SC('_')) ) {
			if( i==_SC('-'))
				iname++;
			else
				break;
		}
		else { iname++; oname++; }
	}

	return i-o;
}

static const encodings_list_tag *find_encoding( const SQChar *name)
{
	const encodings_list_t *enc = encodings_list;

	while( enc->name) {
		if( compare_encoding_name( enc->name, name) == 0) break;
		enc++;
	}

	return enc->name ? enc : NULL;
}

/* ====================================
		Text Reader
==================================== */

#define CBUFF_SIZE	16

static SQInteger SQTextReaderRead( SQUserPointer user, uint8_t *p, SQInteger n);
static SQInteger SQTextReaderWrite( SQUserPointer user, const uint8_t *p, SQInteger n);

struct SQTextReader : public SQStream
{
	SQTextReader( SQStream *stream, SQBool owns) : _converter(SQTextReaderRead,SQTextReaderWrite,this) {
		_stream = stream;
		_owns = owns;
		_buf_len = 0;
		_buf_pos = 0;
	}

	SQInteger Read( void *buffer, SQInteger size) {
		SQInteger preread = 0;
		if( _buf_pos > 0) {								// have something left in _buf
			SQInteger left = _buf_len - _buf_pos;
			SQInteger toread = left;
			if( toread > size)
				toread = size;
			memcpy( buffer, _buf + _buf_pos, toread);
			if( left == toread)							// buffer is empty now
				_buf_pos = 0;
			else
				_buf_pos += toread;
			if( toread == size)							// no more to read
				return size;
			preread = toread;
			size -= toread;
			buffer = (uint8_t*)buffer + toread;
		}												// nothing in _buf
		while( size > 0) {
			uint32_t wc;
			_buf_len = 0;
			if( SQ_FAILED(_converter.ReadChar( &wc))) return preread;
			if( SQ_FAILED(_converter.WriteChar( wc))) return preread;
			SQInteger toread = _buf_len;
			if( toread < size) {
				toread = size;
				_buf_pos = size;
			}
			memcpy( buffer, _buf, toread);
			preread += toread;
			size -= toread;
			buffer = (uint8_t*)buffer + toread;
		}
		return preread;
	}

	SQInteger cnvWrite( const uint8_t *p, SQInteger n)
	{
		if( (_buf_len + n) > CBUFF_SIZE) n = CBUFF_SIZE - _buf_len;
		memcpy( _buf + _buf_len, p, n);
		_buf_len += n;
		return n;
	}

	SQInteger cnvRead( uint8_t *p, SQInteger n)
	{
		return _stream->Read( p, n);
	}
	
protected:
	TextConverter _converter;
	SQStream *_stream;
	SQBool _owns;
	SQInteger _buf_len;
	SQInteger _buf_pos;
	uint8_t _buf[CBUFF_SIZE];
};

SQInteger SQTextReaderRead( SQUserPointer user, uint8_t *p, SQInteger n)
{
	SQTextReader *rdr = (SQTextReader*)user;
	return rdr->cnvRead( p, n);
}

SQInteger SQTextReaderWrite( SQUserPointer user, const uint8_t *p, SQInteger n)
{
	SQTextReader *rdr = (SQTextReader*)user;
	return rdr->cnvWrite( p, n);
}

/* ====================================
		Text Writer
==================================== */

static SQInteger SQTextWriterRead( SQUserPointer user, uint8_t *p, SQInteger n);
static SQInteger SQTextWriterWrite( SQUserPointer user, const uint8_t *p, SQInteger n);

struct SQTextWriter : public SQStream
{
	SQTextReader( SQStream *stream, SQBool owns) : _converter(SQTextWriterRead,SQTextWriterWrite,this) {
		_stream = stream;
		_owns = owns;
	}

    SQInteger Write(void *buffer, SQInteger size) {
		_in_buf = (uint8_t*)buffer;
		_in_size = size;
		
		while( _in_size > 0)
		{
			uint32_t wc;
			if( SQ_FAILED(_converter.ReadChar( &wc))) return preread;
			if( SQ_FAILED(_converter.WriteChar( wc))) return preread;
		}
		
		if( (size - _in_size) < CBUFF_SIZE) {
			memcpy( _tmp_buf, _in_buf, _in_size);
			_tmp_len = _in_size;
			return size;
		}
		return size - _in_size;
	}

	SQInteger cnvRead( uint8_t *p, SQInteger n)
	{
		SQInteger total = 0;
		if( _tmp_len > 0) {
			if( n <= _tmp_len) {
				memcpy( p, _tmp_buf, n);
				_tmp_len -= n;
				return n;
			}
			else {
				memcpy( p, _tmp_buf, _tmp_len);
				n -= _tmp_len;
				p += _tmp_len;
				total = _tmp_len;
				_tmp_len = 0;
			}
		}
		if( n <= _in_size) {
			memcpy( p, _in_buf, n);
			return total + 
		}
		return SQ_ERROR;
	}
	
	SQInteger cnvWrite( const uint8_t *p, SQInteger n)
	{
		return _stream->Write( p, n);
	}
	
protected:
	TextConverter _converter;
	SQStream *_stream;
	SQBool _owns;
};

SQInteger SQTextWriterRead( SQUserPointer user, uint8_t *p, SQInteger n)
{
	SQTextWriter *wrt = (SQTextWriter*)user;
	return wrt->cnvRead( p, n);
}

SQInteger SQTextWriterWrite( SQUserPointer user, const uint8_t *p, SQInteger n)
{
	SQTextWriter *wrt = (SQTextWriter*)user;
	return wrt->cnvWrite( p, n);
}

