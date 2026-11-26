// Character conversion used by the POSIX engine string wrappers.
#ifndef TIER1_PORTABLE_ICONV_H
#define TIER1_PORTABLE_ICONV_H

#if defined( __ANDROID__ )
// Bionic introduced iconv at API 28. SDL's converter also supports the older
// Android versions used by the launcher, without a new runtime dependency.
#include "SDL_stdinc.h"
typedef SDL_iconv_t SourceIconvHandle;
inline SourceIconvHandle SourceIconvOpen( const char *to, const char *from )
{
	return SDL_iconv_open( to, from );
}
inline int SourceIconvClose( SourceIconvHandle handle ) { return SDL_iconv_close( handle ); }
inline size_t SourceIconv( SourceIconvHandle handle, char **input, size_t *inputBytes,
	char **output, size_t *outputBytes )
{
	const char *cursor = *input;
	size_t result = SDL_iconv( handle, &cursor, inputBytes, output, outputBytes );
	*input = const_cast< char * >( cursor );
	return result;
}
#else
#include <iconv.h>
typedef iconv_t SourceIconvHandle;
inline SourceIconvHandle SourceIconvOpen( const char *to, const char *from )
{
	return iconv_open( to, from );
}
inline int SourceIconvClose( SourceIconvHandle handle ) { return iconv_close( handle ); }
inline size_t SourceIconv( SourceIconvHandle handle, char **input, size_t *inputBytes,
	char **output, size_t *outputBytes )
{
	return iconv( handle, input, inputBytes, output, outputBytes );
}
#endif

inline bool SourceIconvIsValid( SourceIconvHandle handle )
{
	return handle != ( SourceIconvHandle )-1 && handle != NULL;
}
#endif
