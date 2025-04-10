#pragma once

#define LogInfo(format, ...) blog(LOG_INFO, "[thunderbolt-share] " format, ##__VA_ARGS__)
#define LogWarning(format, ...) blog(LOG_WARNING, "[thunderbolt-share] " format, ##__VA_ARGS__)

class CBitmapImage
{
public:
    static DWORD ComputeImageSize(UINT width, UINT height, UINT bitsPerPixel);
    static DWORD ComputeImageRowSize(UINT width, UINT bitsPerPixel);
};

class CFileUtil
{
public:
    static BOOL ReadEntireBuffer(HANDLE hFile, BYTE* buffer, UINT32 bufferLen, UINT32& bytesRead, HANDLE hCanceled);
};

class CByteArray
{
public:
    CByteArray();
    CByteArray(size_t Length);
    CByteArray(const BYTE* Buf, size_t Length);

    virtual ~CByteArray();

    __declspec(property(get = get_Buffer)) BYTE* Buffer;
    BYTE* get_Buffer() { return m_Buffer; }

    __declspec(property(get = get_Bytes, put = set_Bytes)) BYTE Bytes[];
    virtual BYTE get_Bytes(size_t index);
    virtual VOID set_Bytes(size_t index, BYTE value);

    __declspec(property(get = get_Length)) size_t Length;
    size_t get_Length() { return m_Length; }

    void Reallocate(size_t Length);

protected:
    BYTE* m_Buffer;
    size_t m_Length;
    size_t m_LengthAllocated;

    void FreeBuffer();
};

#define MAX(x,y) ((x > y) ? x : y)
#define MIN(x,y) ((x < y) ? x : y)

#define ASSERT(x)