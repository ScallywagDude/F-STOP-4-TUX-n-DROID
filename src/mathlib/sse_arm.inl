// Portable implementations of the legacy scalar entry points. Packed SIMD
// operations use sse2neon through the public math headers.
float _SSE_Sqrt( float x ) { return sqrtf( x ); }
float _SSE_RSqrtAccurate( float x ) { return 1.0f / sqrtf( x ); }
float _SSE_RSqrtFast( float x ) { return FastRSqrtFast( x ); }

float FASTCALL _SSE_VectorNormalize( Vector &vec )
{
	return VectorNormalize( vec );
}

void FASTCALL _SSE_VectorNormalizeFast( Vector &vec )
{
	VectorNormalize( vec );
}

float _SSE_InvRSquared( const float *v )
{
	float squared = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	return squared < 1.0f ? 1.0f : 1.0f / squared;
}

void FastSinCos( float x, float *s, float *c )
{
	if ( s )
		*s = sinf( x );
	if ( c )
		*c = cosf( x );
}

float FastCos( float x ) { return cosf( x ); }
void _SSE_SinCos( float x, float *s, float *c ) { FastSinCos( x, s, c ); }
void _SSE2_SinCos( float x, float *s, float *c ) { FastSinCos( x, s, c ); }
float _SSE_cos( float x ) { return cosf( x ); }
float _SSE2_cos( float x ) { return cosf( x ); }

void VectorTransformSSE( const float *input, const matrix3x4_t &matrix, float *output )
{
	float result[3];
	for ( int i = 0; i < 3; ++i )
		result[i] = input[0] * matrix[i][0] + input[1] * matrix[i][1] + input[2] * matrix[i][2] + matrix[i][3];
	for ( int i = 0; i < 3; ++i )
		output[i] = result[i];
}

void VectorRotateSSE( const float *input, const matrix3x4_t &matrix, float *output )
{
	float result[3];
	for ( int i = 0; i < 3; ++i )
		result[i] = input[0] * matrix[i][0] + input[1] * matrix[i][1] + input[2] * matrix[i][2];
	for ( int i = 0; i < 3; ++i )
		output[i] = result[i];
}
