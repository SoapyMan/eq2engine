//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Virtual Stream
//////////////////////////////////////////////////////////////////////////////////

#include "VirtualStream.h"
#include "core/IFileSystem.h"
#include "core/platform/MessageBox.h"
#include "utils/CRC32.h"

#include <stdarg.h> // va_*
#include <stdio.h>

#define VSTREAM_GRANULARITY 1024	// 1kb

// Opens memory stream, when creating new stream use nBufferSize parameter as base buffer
IVirtualStream* OpenMemoryStream(int nOpenFlags, int nBufferSize, ubyte* pBufferData)
{
	CMemoryStream* pMemoryStream = new CMemoryStream;

	if(pMemoryStream->Open(pBufferData, nOpenFlags, 0))
	{
		return pMemoryStream;
	}
	else
	{
		delete pMemoryStream;
		return NULL;
	}
}

// prints string to stream
void IVirtualStream::Print(const char* pFmt, ...)
{
	va_list		argptr;

	static char	string[4096];

	va_start (argptr,pFmt);
	int wcount = vsprintf(string, pFmt, argptr);
	va_end (argptr);

	Write(string, 1, wcount);
}

//--------------------------
// CMemoryStream - File stream
//--------------------------

CMemoryStream::CMemoryStream()
{
	m_pCurrent = NULL;
	m_pStart = NULL;

	m_nAllocatedSize = 0;
	m_nUsageFlags = 0;
}

// destroys stream data
CMemoryStream::~CMemoryStream()
{
	Close();

	// destroy memory
	m_pCurrent = NULL;

	PPFree( m_pStart );
	m_pStart = NULL;
	m_nAllocatedSize = 0;
	m_nUsageFlags = 0;
}

// reads data from virtual stream
size_t CMemoryStream::Read(void *dest, size_t count, size_t size)
{
	if(!(m_nUsageFlags & VS_OPEN_READ) || m_nAllocatedSize == 0)
		return 0;

	size_t nReadBytes = size*count;

	long nCurPos = Tell();

	if(nCurPos+nReadBytes > m_nAllocatedSize)
		nReadBytes -= ((nCurPos+nReadBytes) - m_nAllocatedSize);

	// copy memory
	memcpy(dest, m_pCurrent, nReadBytes);

	m_pCurrent += nReadBytes;

	return nReadBytes;
}

// writes data to virtual stream
size_t CMemoryStream::Write(const void *src, size_t count, size_t size)
{
	if(!(m_nUsageFlags & VS_OPEN_WRITE))
		return 0;

	long nAddBytes = size*count;

	long nCurrPos = Tell();

	if(nCurrPos+nAddBytes > m_nAllocatedSize)
	{
		long mem_diff = (nCurrPos+nAddBytes) - m_nAllocatedSize;

		long newSize = m_nAllocatedSize + mem_diff + VSTREAM_GRANULARITY - 1;
		newSize -= newSize % VSTREAM_GRANULARITY;

		ReAllocate( newSize );
	}

	// copy memory
	memcpy(m_pCurrent, src, nAddBytes);

	m_pCurrent += nAddBytes;

	return count;
}

// seeks pointer to position
int CMemoryStream::Seek(long nOffset, VirtStreamSeek_e seekType)
{
	switch(seekType)
	{
		case VS_SEEK_SET:
			m_pCurrent = m_pStart+nOffset;
			break;
		case VS_SEEK_CUR:
			m_pCurrent = m_pCurrent+nOffset;
			break;
		case VS_SEEK_END:
			m_pCurrent = m_pStart + m_nAllocatedSize + nOffset;
			break;
	}

	return Tell();
}

// returns current pointer position
long CMemoryStream::Tell()
{
	return m_pCurrent - m_pStart;
}

// returns memory allocated for this stream
long CMemoryStream::GetSize()
{
	return m_nAllocatedSize;
}

// opens stream, if this is a file, data is filename
bool CMemoryStream::Open(ubyte* data, int nOpenFlags, int nDataSize)
{
	Close();

	m_nUsageFlags = nOpenFlags;

	if((m_nUsageFlags & VS_OPEN_WRITE) && !(m_nUsageFlags & VS_OPEN_READ))
	{
		// data will be written reset it
		if(m_pStart)
			PPFree(m_pStart);

		m_nAllocatedSize = 0;

		m_pStart = NULL;
		m_pCurrent = NULL;
	}

	if((m_nUsageFlags & VS_OPEN_MEMORY_FROM_EXISTING) && (m_nUsageFlags & VS_OPEN_READ) && data)
	{
		m_pStart = data;
		m_pCurrent = m_pStart;
		m_nAllocatedSize = nDataSize;
	}
	else
	{
		// make this as base buffer
		if(nDataSize > 0 && m_pStart == NULL)
			ReAllocate(nDataSize);

		if((m_nUsageFlags & VS_OPEN_READ) && data)
		{
			memcpy(m_pStart, data, nDataSize);
		}
	}

	return true;
}

// closes stream
void CMemoryStream::Close()
{
	if((m_nUsageFlags & VS_OPEN_READ) && (m_nUsageFlags & VS_OPEN_MEMORY_FROM_EXISTING))
		m_pStart = NULL;

	m_nUsageFlags = 0;
	m_pCurrent = m_pStart;
}

// flushes stream, doesn't affects on memory stream
int CMemoryStream::Flush()
{
	// do nothing
	return 0;
}

// reallocates memory
void CMemoryStream::ReAllocate(long nNewSize)
{
	if(nNewSize == m_nAllocatedSize)
		return;

	// don't reallocate existing buffer
	ASSERT(!(m_nUsageFlags & VS_OPEN_MEMORY_FROM_EXISTING));

	if(m_nUsageFlags & VS_OPEN_MEMORY_FROM_EXISTING)
		return; // bail out!

	long curPos = Tell();

	ubyte* pTemp = (ubyte*)PPAlloc( nNewSize );

	if(m_pStart && m_nAllocatedSize > 0)
	{
		memcpy( pTemp, m_pStart, m_nAllocatedSize );

		PPFree(m_pStart);
	}

	m_nAllocatedSize = nNewSize;

	m_pStart = pTemp;
	m_pCurrent = m_pStart+curPos;

	//Msg("CMemoryStream::ReAllocate(): New size: %d\n", nNewSize);
}

// saves stream to file for stream (only for memory stream )
void CMemoryStream::WriteToFileStream(IFile* pFile)
{
	int stream_size = m_pCurrent-m_pStart;

	pFile->Write(m_pStart, 1, stream_size);
}

// reads file to this stream
bool CMemoryStream::ReadFromFileStream( IFile* pFile )
{
	if(!(m_nUsageFlags & VS_OPEN_WRITE))
		return false;

	int rest_pos = pFile->Tell();
	int filesize = pFile->GetSize();
	pFile->Seek(0, VS_SEEK_SET);

	ReAllocate( filesize + 32 );

	// read to me
	pFile->Read(m_pStart, filesize, 1);

	pFile->Seek(rest_pos, VS_SEEK_SET);

	// let user seek this stream after
	return true;
}

// returns current pointer to the stream (only memory stream)
ubyte* CMemoryStream::GetCurrentPointer()
{
	return m_pCurrent;
}

// returns base pointer to the stream (only memory stream)
ubyte* CMemoryStream::GetBasePointer()
{
	return m_pStart;
}

uint32 CMemoryStream::GetCRC32()
{
	uint32 nCRC = CRC32_BlockChecksum( m_pStart, Tell() );

	return nCRC;
}
