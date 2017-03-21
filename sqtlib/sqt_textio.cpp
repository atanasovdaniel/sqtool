
#include <squirrel.h>
#include <sqtool.h>
#include <sqstreamreader.h>



struct SQTTextReader
{
	SQTTextReader( SQFILE stream, sqBool owns) { _stream = stream; _owns = owns };
	SQTTextReader( SQFILE stream, const SQString *encoding, SQBool guess, sqBool owns) { _stream = stream };
	
	void Close() {
		if( _owns && _stream) {
			sqstd_fclose( _stream);
		}
		_stream = NULL;
		_owns = SQFlase;
	}
	
	SQInteger ReadChar() {
		
	}
	
	SQFILE _stream;
	SQBool _owns;
};