{
	local a = [];
	foreach( n,v in this) a.append(n);
	a.sort();
	foreach( n in a) print( n + ":" + this[n] + "\n");
}
