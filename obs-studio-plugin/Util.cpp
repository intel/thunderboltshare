#include "pch.h"
#include "Util.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DWORD CBitmapImage::ComputeImageSize(UINT width, UINT height, UINT bitsPerPixel)
{
    if (bitsPerPixel < 8)
    {
        bitsPerPixel = 8;
    }

    if (bitsPerPixel == 15)
    {
        bitsPerPixel = 16;
    }

    return ((bitsPerPixel * width + 31) / 32) * 4 * height;
}

DWORD CBitmapImage::ComputeImageRowSize(UINT width, UINT bitsPerPixel)
{
    if (bitsPerPixel < 8)
    {
        bitsPerPixel = 8;
    }

    if (bitsPerPixel == 15)
    {
        bitsPerPixel = 16;
    }

    return ((bitsPerPixel * width + 31) / 32) * 4;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFileUtil::ReadEntireBuffer(HANDLE hFile, BYTE* buffer, UINT32 bufferLen, UINT32& bytesRead, HANDLE hCanceled)
{
    BOOL result = FALSE;
    bytesRead = 0;

    ULONG totalBytesRemaining = bufferLen;
    ULONG totalBytesRead = 0;

    PUCHAR pBuf = buffer;

    while ((totalBytesRemaining > 0) &&
           (WaitForSingleObject(hCanceled, 0) != WAIT_OBJECT_0))
    {
        ULONG currentBytesRead = 0;

        if (!ReadFile(hFile, pBuf + totalBytesRead, totalBytesRemaining, &currentBytesRead, NULL))
        {
            bytesRead = totalBytesRead;
            return FALSE;
        }

        if (currentBytesRead == 0)
        {
            //
            // We've got to stop, we've reached the end of the file, or the pipe was closed
            //
            bytesRead = totalBytesRead;
            return FALSE;
        }

        totalBytesRead += currentBytesRead;

        if (totalBytesRemaining < currentBytesRead)
        {
            bytesRead = totalBytesRead;
            return FALSE;
        }
        else
        {
            totalBytesRemaining -= currentBytesRead;
        }
    }

    bytesRead = totalBytesRead;
    return (totalBytesRemaining == 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define AlignedMalloc _aligned_malloc
#define AlignedFree _aligned_free
#define MemoryCopyDefault memcpy
#define BYTE_ARRAY_ALIGNMENT 512

CByteArray::CByteArray()
    : m_Buffer(NULL),
    m_Length(0),
    m_LengthAllocated(0)
{
}

CByteArray::CByteArray(size_t Length)
    : m_Buffer(NULL),
    m_Length(Length),
    m_LengthAllocated(Length)
{
    if (Length > 0)
    {
        m_Buffer = (BYTE*)AlignedMalloc(Length, BYTE_ARRAY_ALIGNMENT);
    }
}

CByteArray::CByteArray(const BYTE* Buf, size_t Length)
    : m_Buffer(NULL),
    m_Length(Length),
    m_LengthAllocated(Length)
{
    if (Length > 0)
    {
        m_Buffer = (BYTE*)AlignedMalloc(Length, BYTE_ARRAY_ALIGNMENT);
        MemoryCopyDefault(m_Buffer, Buf, Length);
    }
}

CByteArray::~CByteArray()
{
    FreeBuffer();
}

void CByteArray::FreeBuffer()
{
    AlignedFree(m_Buffer);
    m_Length = 0;
    m_LengthAllocated = 0;
    m_Buffer = NULL;
}

BYTE CByteArray::get_Bytes(size_t index)
{
    if ((index < 0) || (index >= m_Length))
    {
        ASSERT(FALSE);
    }

    return m_Buffer[index];
}

VOID CByteArray::set_Bytes(size_t index, BYTE value)
{
    if ((index < 0) || (index >= m_Length))
    {
        ASSERT(FALSE);
    }

    m_Buffer[index] = value;
}

void CByteArray::Reallocate(size_t Length)
{
    if (Length > m_LengthAllocated)
    {
        FreeBuffer();

        if (Length > 0)
        {
            m_Buffer = (BYTE*)AlignedMalloc(Length, BYTE_ARRAY_ALIGNMENT);
        }

        m_LengthAllocated = Length;
    }

    //
    // Note that if Length == 0, we may have a Buffer still allocated, but a "Length" of 0.
    //
    m_Length = Length;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
