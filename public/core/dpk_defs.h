///////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Dark package file (dpk) definitions
//////////////////////////////////////////////////////////////////////////////////

#ifndef DPK_DEFS_H
#define DPK_DEFS_H

#include "dktypes.h"
#include "utils/eqstring.h"

#define DPK_VERSION					6
#define DPK_SIGNATURE				MCHAR4('E','Q','P','K')

#define DPK_BLOCK_MAXSIZE			(8*1024)
#define DPK_STRING_SIZE				255

enum EFileFlags
{
	DPKFILE_FLAG_COMPRESSED			= (1 << 0),
	DPKFILE_FLAG_ENCRYPTED			= (1 << 1),
};

//---------------------------

// data package header
struct dpkheader_s
{
public:
	int		signature;

	uint8	version;
	uint8	compressionLevel;

	int		numFiles;
	uint64	fileInfoOffset;
};
ALIGNED_TYPE(dpkheader_s, 2) dpkheader_t;

//---------------------------

struct dpkblock_s
{
	uint32	size;
	uint32	compressedSize;
	short	flags;
};
ALIGNED_TYPE(dpkblock_s, 2) dpkblock_t;

// data package file info
struct dpkfileinfo_s
{
	int		filenameHash;

	uint64	offset;
	uint32	size;				// The real file size

	short	numBlocks;			// number of blocks

	short	flags;
};
ALIGNED_TYPE(dpkfileinfo_s, 2) dpkfileinfo_t;

// dpk path fix
void DPK_FixSlashes(EqString& str);

#endif //DPK_DEFS_H
