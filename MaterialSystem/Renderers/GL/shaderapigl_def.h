//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLRENDERER_CONSTANTS_H
#define GLRENDERER_CONSTANTS_H

static const GLuint internalFormats[FORMAT_COUNT] = {
	0,
	// Unsigned formats
#ifdef USE_GLES2
	0, //GL_INTENSITY8,
	0, //GL_LUMINANCE8_ALPHA8,
#else
	GL_INTENSITY8,
	GL_LUMINANCE8_ALPHA8,
#endif // USE_GLES2
	GL_RGB8,
	GL_RGBA8,
#ifdef USE_GLES2
	0, //GL_INTENSITY16,
	0, //GL_LUMINANCE16_ALPHA16,
	GL_RGB16I,
	GL_RGBA16I,
#else
	GL_INTENSITY16,
	GL_LUMINANCE16_ALPHA16,
	GL_RGB16,
	GL_RGBA16,
#endif // USE_GLES2

	// Signed formats
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,
#ifdef USE_GLES2
	// Float formats
	0, // GL_INTENSITY16F_ARB,
	0, //GL_LUMINANCE_ALPHA16F_ARB,
	GL_RGB16F,
	GL_RGBA16F,
	0, //GL_INTENSITY32F_ARB,
	0, //GL_LUMINANCE_ALPHA32F_ARB,
	GL_RGB32F,
	GL_RGBA32F,
#else
	// Float formats
	GL_INTENSITY16F_ARB,
	GL_LUMINANCE_ALPHA16F_ARB,
	GL_RGB16F_ARB,
	GL_RGBA16F_ARB,
	GL_INTENSITY32F_ARB,
	GL_LUMINANCE_ALPHA32F_ARB,
	GL_RGB32F_ARB,
	GL_RGBA32F_ARB,
#endif // USE_GLES2

	// Signed integer formats
	0, // GL_INTENSITY16I_EXT,
	0, // GL_LUMINANCE_ALPHA16I_EXT,
	0, // GL_RGB16I_EXT,
	0, // GL_RGBA16I_EXT,

	0, // GL_INTENSITY32I_EXT,
	0, // GL_LUMINANCE_ALPHA32I_EXT,
	0, // GL_RGB32I_EXT,
	0, // GL_RGBA32I_EXT,

	// Unsigned integer formats
	0, // GL_INTENSITY16UI_EXT,
	0, // GL_LUMINANCE_ALPHA16UI_EXT,
	0, // GL_RGB16UI_EXT,
	0, // GL_RGBA16UI_EXT,

	0, // GL_INTENSITY32UI_EXT,
	0, // GL_LUMINANCE_ALPHA32UI_EXT,
	0, // GL_RGB32UI_EXT,
	0, // GL_RGBA32UI_EXT,

	// Packed formats
	0, // RGBE8 not directly supported
	0, // GL_RGB9_E5,
	0, // GL_R11F_G11F_B10F,
#ifdef USE_GLES2
	0, // GL_RGB5,
#else
	GL_RGB5,
#endif // USE_GLES2
	GL_RGBA4,
	GL_RGB10_A2,

	// Depth formats
	GL_DEPTH_COMPONENT16,
	GL_DEPTH_COMPONENT24,
	GL_DEPTH24_STENCIL8,
	0, // GL_DEPTH_COMPONENT32F,

	// Compressed formats
#ifdef USE_GLES2
#ifdef _WIN32 // temporary: ANGLE
	0x83F0, //0, //GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
	0x83F2, //0, //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	0x83F3, //0, //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
#else
	0, // GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
	0, // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	0, // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
#endif // _WIN32
	0, // ATI1N not yet supported
	0,// GL_COMPRESSED_LUMINANCE_ALPHA,

	0x8D64, // which is GL_COMPRESSED_RGB8_ETC1
	GL_COMPRESSED_RGB8_ETC2,
	GL_COMPRESSED_RGBA8_ETC2_EAC,
	0, //GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,
	0, //GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,
	0, //GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,
	0, //GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,
#else
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
	0, // ATI1N not yet supported
	GL_COMPRESSED_LUMINANCE_ALPHA,			//COMPRESSED_LUMINANCE_ALPHA_3DC_ATI,

	0, // GL_COMPRESSED_RGB8_ETC1
	0, // GL_COMPRESSED_RGB8_ETC2,
	0, // GL_COMPRESSED_RGBA8_ETC2_EAC,
	0, // GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,
	0, // GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,
	0, // GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,
	0, // GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,
#endif // USE_GLES2
};

static inline GLuint PickGLInternalFormat(ETextureFormat format)
{
	GLint internalFormat = internalFormats[format];

	if (format >= FORMAT_I32F && format <= FORMAT_RGBA32F)
		internalFormat = internalFormats[format - (FORMAT_I32F - FORMAT_I16F)];

	return internalFormat;
}

static const GLuint chanCountTypes[] = { 0, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA };

static const GLuint chanTypePerFormat[] = {
	0,

	// Unsigned formats
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_BYTE,

	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_SHORT,

	// Signed formats
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	// Float formats
#ifdef USE_GLES2
	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
	GL_HALF_FLOAT,
#else
	GL_HALF_FLOAT_ARB,
	GL_HALF_FLOAT_ARB,
	GL_HALF_FLOAT_ARB,
	GL_HALF_FLOAT_ARB,
#endif // USE_GLES2

	GL_FLOAT,
	GL_FLOAT,
	GL_FLOAT,
	GL_FLOAT,

	// Signed integer formats
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	// Unsigned integer formats
	0,
	0,
	0,
	0,

	0,
	0,
	0,
	0,

	// Packed formats
	0, // RGBE8 not directly supported
	0, // RGBE9E5 not supported
	0, // RG11B10F not supported
	GL_UNSIGNED_SHORT_5_6_5,
#ifdef USE_GLES2
	0, // GL_UNSIGNED_SHORT_4_4_4_4 ???,
#else
	GL_UNSIGNED_SHORT_4_4_4_4_REV,
#endif // USE_GLES2
	GL_UNSIGNED_INT_2_10_10_10_REV,

	// Depth formats
	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_INT,
	GL_UNSIGNED_INT_24_8,
	0, // D32F not supported

	// Compressed formats
#ifdef USE_GLES2
	0, //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	0, //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	0, //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
#else
	GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
	GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
#endif // USE_GLES2

	0,//(D3DFORMAT) '1ITA', // 3Dc 1 channel
	0,//(D3DFORMAT) '2ITA', // 3Dc 2 channels

#ifdef USE_GLES2
	0x8D64, // which is GL_COMPRESSED_RGB8_ETC1
	GL_COMPRESSED_RGB8_ETC2,
	GL_COMPRESSED_RGBA8_ETC2_EAC,
	0, //GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,
	0, //GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,
	0, //GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,
	0, //GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,
#else
	0, // GL_COMPRESSED_RGB8_ETC1
	0, // GL_COMPRESSED_RGB8_ETC2,
	0, // GL_COMPRESSED_RGBA8_ETC2_EAC,
	0, // GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG,
	0, // GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG,
	0, // GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG,
	0, // GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG,
#endif // #ifdef USE_GLES2

};

const GLenum addressModes[] = {
	GL_REPEAT,
	GL_CLAMP_TO_EDGE,
	GL_MIRRORED_REPEAT
};

static const GLenum blendingConsts[] = {
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_SRC_ALPHA_SATURATE,
};

static const GLenum blendingModes[] = {
	GL_FUNC_ADD,
	GL_FUNC_SUBTRACT,
	GL_FUNC_REVERSE_SUBTRACT,
	GL_MIN,
	GL_MAX,
};

static const GLenum depthConst[] = {
	GL_NEVER,
	GL_LESS,
	GL_EQUAL,
	GL_LEQUAL,
	GL_GREATER,
	GL_NOTEQUAL,
	GL_GEQUAL,
	GL_ALWAYS,
};

static const GLenum stencilConst[] = {
	GL_KEEP,
	GL_ZERO,
	GL_REPLACE,
	GL_INVERT,
	GL_INCR_WRAP,
	GL_DECR_WRAP,
	GL_MAX,
	GL_INCR,
	GL_DECR,
	GL_ALWAYS,
};

static const GLenum cullConst[] = {
	GL_NONE,
	GL_BACK,
	GL_FRONT,
};

#ifndef USE_GLES2
static const GLenum fillConst[] = {
	GL_FILL,
	GL_LINE,
	GL_POINT,
};
#endif // USE_GLES2

static const GLenum minFilters[] = {
	GL_NEAREST,
	GL_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_LINEAR,
};

#ifndef USE_GLES2
static const GLenum matrixModeConst[] = {
	GL_MODELVIEW,
	GL_PROJECTION,
	GL_MODELVIEW,
	GL_MODELVIEW,
	GL_TEXTURE,
};
#endif // USE_GLES2

// for some speedup
static const GLenum mbTypeConst[] = {
	GL_POINTS,
	GL_LINES,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
#ifdef USE_GLES2
	0, // GL_POLYGON,
	0, // GL_QUADS,
#else
	GL_POLYGON,
	GL_QUADS,
#endif // USE_GLES2
};

static const GLenum glPrimitiveType[] = {
	GL_TRIANGLES,
	GL_TRIANGLE_FAN,
	GL_TRIANGLE_STRIP,
#ifdef USE_GLES2
	0, // GL_QUADS,
#else
	GL_QUADS,
#endif // USE_GLES2
	GL_LINES,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
	GL_POINTS,
};

static const GLenum glBufferUsages[] = {
	GL_STREAM_DRAW,
	GL_STATIC_DRAW,
	GL_DYNAMIC_DRAW,
};

// map to EGLTextureType
static const GLenum glTexTargetType[] = {
	GL_TEXTURE_2D,
	GL_TEXTURE_CUBE_MAP,
	GL_TEXTURE_3D,
};

#ifdef USE_GLES2

#define glDrawElementsInstancedARB glDrawElementsInstanced
#define glDrawArraysInstancedARB glDrawArraysInstanced
#define glVertexAttribDivisorARB glVertexAttribDivisor

#define GL_OBJECT_COMPILE_STATUS_ARB GL_COMPILE_STATUS
#define GL_OBJECT_COMPILE_STATUS_ARB GL_COMPILE_STATUS
#define GL_OBJECT_LINK_STATUS_ARB GL_LINK_STATUS

#define GL_OBJECT_ACTIVE_UNIFORMS_ARB GL_ACTIVE_UNIFORMS
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB GL_ACTIVE_UNIFORM_MAX_LENGTH

#endif // USE_GLES2

#endif //GLRENDERER_CONSTANTS_H
