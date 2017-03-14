/* see copyright notice in sqtool.h */
#ifndef _SQTOOL_OSNAME_H_
#define _SQTOOL_OSNAME_H_

#define SQT_OSNAME_WINDOWS	"windows"
#define SQT_OSNAME_LINUX	"linux"
#define SQT_OSNAME_UNKNOWN	"unknown"

#if defined(_WIN32)
	#define SQT_OSNAME	SQT_OSNAME_WINDOWS
#elif defined(__linux__)
	#define SQT_OSNAME	SQT_OSNAME_LINUX
#else
	#define SQT_OSNAME	SQT_OSNAME_UNKNOWN
#endif

#endif // _SQTOOL_OSNAME_H_
