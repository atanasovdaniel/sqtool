
function blob::dump()
{
	local l = len();
	local i;
	local r = "";
	for( i=0; i < l; i++)
	{
		r += " " + this[i];
	}
	::print( r + "\n");
}

B <- blob();
{
	local i;
	for( i=0; i < 50; i++)
		B.writen(i,'b');
	B.seek(0);
}
//B.dump();

function test_1( buff_size)
{
	B.seek(0);
	local s = streamreader( B, false, buff_size);
	local b;
	local r;
	
	// read 3 bytes: 0 1 2
	b = s.readblob(3);
	b.dump();
	if( (b[0] != 0) || (b[2] != 2)) print( "!!! Bad content\n");
	
	// put mark for 5 bytes
	r = s.mark(5);
	print( "mark(5) -> " + r + "\n");
	if( r) print( "!!! Bad mark() result\n");
	
	// read next 3 bytes: 3 4 5
	b = s.readblob(3);
	b.dump();
	if( (b[0] != 3) || (b[2] != 5)) print( "!!! Bad content\n");
	
	// reset to mark
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( r) print( "!!! Bad reset() result\n");
	
	// read 5 bytes: 3, 4, 5, 6, 7
	b = s.readblob(5);
	b.dump();
	if( (b[0] != 3) || (b[4] != 7)) print( "!!! Bad content\n");

	// reset to mark
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( r) print( "!!! Bad reset() result\n");
	
	// read 6 bytes: 3, 4, 5, 6, 7, 8
	b = s.readblob(6);
	b.dump();
	if( (b[0] != 3) || (b[5] != 8)) print( "!!! Bad content\n");

	// reset to mark - mark is gone
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( !r) print( "!!! Bad reset() result\n");
}

function test_2( buff_size)
{
	B.seek(0);
	local s = streamreader( B, false, buff_size);
	local b;
	local r;
	
	// read 3 bytes: 0, 1, 2
	b = s.readblob(3);
	b.dump();
	if( (b[0] != 0) || (b[2] != 2)) print( "!!! Bad content\n");
	
	// put mark for 5 bytes
	r = s.mark(5);
	print( "mark(5) -> " + r + "\n");
	if( r) print( "!!! Bad mark() result\n");
	
	// read next 3 bytes: 3, 4, 5
	b = s.readblob(3);
	b.dump();
	if( (b[0] != 3) || (b[2] != 5)) print( "!!! Bad content\n");

	// put mark for 5 bytes (over unused mark)
	r = s.mark(5);
	print( "mark(5) -> " + r + "\n");
	if( r) print( "!!! Bad mark() result\n");
	
	// read next 5 bytes: 6, 7, 8, 9, 10
	b = s.readblob(5);
	b.dump();
	if( (b[0] != 6) || (b[4] != 10)) print( "!!! Bad content\n");

	// reset to mark
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( r) print( "!!! Bad reset() result\n");
	
	// read again 5 + next 1 bytes: 6, 7, 8, 9, 10, 11
	b = s.readblob(6);
	b.dump();
	if( (b[0] != 6) || (b[5] != 11)) print( "!!! Bad content\n");

	// reset to mark - mark is gone
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( !r) print( "!!! Bad reset() result\n");
}

function test_3( buff_size)
{
	B.seek(0);
	local s = streamreader( B, false, buff_size);
	local b;
	local r;
	
	// read 3 bytes: 0, 1, 2
	b = s.readblob(3);
	b.dump();
	if( (b[0] != 0) || (b[2] != 2)) print( "!!! Bad content\n");
	
	// put mark for 5 bytes
	r = s.mark(5);
	print( "mark(5) -> " + r + "\n");
	if( r) print( "!!! Bad mark() result\n");
	
	// read next 5 bytes: 3, 4, 5, 6, 7
	b = s.readblob(5);
	b.dump();
	if( (b[0] != 3) || (b[4] != 7)) print( "!!! Bad content\n");
	
	// reset to mark
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( r) print( "!!! Bad reset() result\n");
	
	// read 3 bytes: 3, 4, 5
	b = s.readblob(3);
	b.dump();
	if( (b[0] != 3) || (b[2] != 5)) print( "!!! Bad content\n");
	
	// put mark for 5 bytes (inside _mark_buffer)
	r = s.mark(5);
	print( "mark(5) -> " + r + "\n");
	if( r) print( "!!! Bad mark() result\n");
	
	// read next 5 bytes: 6, 7, 8, 9, 10
	b = s.readblob(5);
	b.dump();
	if( (b[0] != 6) || (b[4] != 10)) print( "!!! Bad content\n");
	
	// reset to mark
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( r) print( "!!! Bad reset() result\n");
	
	// read again 5 + next 1 bytes: 6, 7, 8, 9, 10, 11
	b = s.readblob(6);
	b.dump();
	if( (b[0] != 6) || (b[5] != 11)) print( "!!! Bad content\n");

	// reset to mark - mark is gone
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( !r) print( "!!! Bad reset() result\n");
}

function test_4( buff_size)
{
	local b;
	local r;
	local bb = blob(5);
	bb[0] = 0; bb[1] = 1; bb[2] = 2; bb[3] = 3; bb[4] = 4;
	bb.dump();

	bb.seek(0);
	local s = streamreader(bb,false, buff_size);
	
	// put mark for 10 bytes
	r = s.mark(10);
	print( "mark(10) -> " + r + "\n");
	if( r) print( "!!! Bad mark() result\n");
	
	// read 10 bytes - have only 5 
	b = s.readblob(10);
	b.dump();
	if( b.len() != 5) print( "!!! Bad length\n");
	if( (b[0] != 0) || (b[4] != 4)) print( "!!! Bad content\n");
	r = s.eos();
	print( "eos() -> " + r + "\n");
	if( !r) print( "!!! Bad EOS\n");
	
	// reset to mark
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( r) print( "!!! Bad reset() result\n");
	r = s.eos();
	print( "eos() -> " + r + "\n");
	if( r) print( "!!! Bad EOS\n");
	
	// read 3 bytes - 0, 1, 2
	b = s.readblob(3);
	b.dump();
	if( b.len() != 3) print( "!!! Bad length\n");
	if( (b[0] != 0) || (b[2] != 2)) print( "!!! Bad content\n");
	r = s.eos();
	print( "eos() -> " + r + "\n");
	if( r) print( "!!! Bad EOS\n");
	
	// read 2 bytes - 3, 4
	b = s.readblob(3);
	b.dump();
	if( b.len() != 2) print( "!!! Bad length\n");
	if( (b[0] != 3) || (b[1] != 4)) print( "!!! Bad content\n");
	r = s.eos();
	print( "eos() -> " + r + "\n");
	if( !r) print( "!!! Bad EOS\n");
}

function test_5( buff_size)
{
	local b;
	local r;
	local f;
	
	f = file("./test/file_nt.txt","r");
	local s = streamreader(f,false,buff_size);
	
	// put mark for 10 bytes
	r = s.mark(10);
	print( "mark(10) -> " + r + "\n");
	if( r) print( "!!! Bad mark() result\n");
	
	// read 10 bytes - have only 5 "again"
	b = s.readblob(10);
	b.dump();
	if( b.len() != 5) print( "!!! Bad length\n");
	if( (b[0] != 'a') || (b[4] != 'n')) print( "!!! Bad content\n");
	r = s.eos();
	print( "eos() -> " + r + "\n");
	if( !r) print( "!!! Bad EOS\n");
	
	// reset to mark
	r = s.reset();
	print( "reset() -> " + r + "\n");
	if( r) print( "!!! Bad reset() result\n");
	r = s.eos();
	print( "eos() -> " + r + "\n");
	if( r) print( "!!! Bad EOS\n");
	
	// read 3 bytes - "aga"
	b = s.readblob(3);
	b.dump();
	if( b.len() != 3) print( "!!! Bad length\n");
	if( (b[0] != 'a') || (b[2] != 'a')) print( "!!! Bad content\n");
	r = s.eos();
	print( "eos() -> " + r + "\n");
	if( r) print( "!!! Bad EOS\n");
	
	// read 2 bytes - "in"
	b = s.readblob(3);
	b.dump();
	if( b.len() != 2) print( "!!! Bad length\n");
	if( (b[0] != 'i') || (b[1] != 'n')) print( "!!! Bad content\n");
	r = s.eos();
	print( "eos() -> " + r + "\n");
	if( !r) print( "!!! Bad EOS\n");
	
	s.close();
	f.close();
}

function test_6( buff_size)
{
	local s;
	local r;
	local b;

	r = collectgarbage();
	print( "collectgarbage() -> " + r + "\n");
	
	s = streamreader( blob(10), false, buff_size);
	r = collectgarbage();
	print( "collectgarbage() -> " + r + "\n");
	
	local b = s.readblob(3);
	b.dump();
	
	r = s.close();
	print( "close() -> " + r + "\n");
	
	r = collectgarbage();
	print( "collectgarbage() -> " + r + "\n");
	
}

function test_all( buff_size)
{
	print("\n----------------\n");
	print("buff_size=" + buff_size + "\n");
	print("----------------\n");
	
	test_1(buff_size);
	print("\n");
	test_2(buff_size);
	print("\n");
	test_3(buff_size);
	print("\n");
	test_4(buff_size);
	print("\n");
	test_5(buff_size);
	print("\n");
	test_6(buff_size);
}

test_all(0);
test_all(1);
test_all(1000);

