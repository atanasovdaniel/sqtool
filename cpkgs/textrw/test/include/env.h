#ifndef __ENV_H__
#define __ENV_H__

extern void env_dump_buffer( const void *buf, int len);

extern int env_dump_allocs;

extern void env_set_sread_buffer( void *buf, SQInteger len);
extern int env_stream_eof;

extern int env_sqstd_swrite_dump;
extern void env_set_swrite_buffer( void *buf, SQInteger len);

#endif // __ENV_H__
