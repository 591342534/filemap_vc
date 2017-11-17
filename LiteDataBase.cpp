#include "LiteDataBase.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif // _MSC_VER

static std::string GetGuidToString()
{
    GUID guid;
    CoCreateGuid(&guid);

    char buf[64] = { 0 };
    snprintf(buf, sizeof(buf), "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5],
        guid.Data4[6], guid.Data4[7]);
    return std::string(buf);
}


static bool IsExists(const char* path)
{
    if (_access(path, 0) != -1) {
        return true;
    }

    return false;
}

static int64_t AlignUp_Int64(int64_t data, int64_t align) {
    return (data + align - 1) / align * align;
}

static int64_t AlignDown_Int64(int64_t data, int64_t align) {
    return data / align * align;
}







// LiteDataBase

LiteDataBase::LiteDataBase()
{
    mHFileMap = NULL;
    mHMapView = NULL;

    this->mMapViewOffset = 0;
    this->mMapViewSize = 0;
    this->mCurrentOffset = 0;
    this->mTotalSize = 0;
}


bool LiteDataBase::mIsInCurrentMapView(int64_t offset, int32_t size /*= 0*/)
{
    if (size < 0) {
        printf("[%s %d] warning !!!\n");
        size = 0;
    }

    if (NULL != mHMapView && offset >= mMapViewOffset && offset + size <= mMapViewOffset + mMapViewSize){
        return true;
    }

    return false;
}



bool LiteDataBase::mUpdateMapView(int64_t offset)
{
    if (mHMapView) {
        UnmapViewOfFile(mHMapView);
        mHMapView = NULL;
    }

    // ���MapVew�ѵ�FileMap��β��, ѭ��ʹ��
    if (offset + mMapViewSize > mTotalSize) {
        offset = 0;
    }

    LARGE_INTEGER largeOffset;
    largeOffset.QuadPart = offset;

    mHMapView = MapViewOfFile(this->mHFileMap,
        dwDesiredAccess, /*FILE_MAP_ALL_ACCESS*/
        largeOffset.HighPart,
        largeOffset.LowPart,
        mMapViewSize);

    if (NULL == mHMapView) {
        printf("MapViewOfFile error! %d\n", GetLastError());
        return false;
    }

    printf("\nӳ��%s: [%10I64d, %10I64d] bytes ֮�������\n\n", dwDesiredAccess&FILE_MAP_READ ? "��" : "д", offset, offset + mMapViewSize);

    this->mMapViewOffset = offset;
    this->mCurrentOffset = 0;

    return true;
}












// LiteDataBaseWrite

LiteDataBaseWrite::LiteDataBaseWrite()
{
    mHFile = NULL;
    this->dwDesiredAccess = FILE_MAP_WRITE;
}


LiteDataBaseWrite::~LiteDataBaseWrite()
{
    this->DeInitWrite();
}


bool LiteDataBaseWrite::InitWrite(int64_t totalSize, int32_t mapViewSize)
{
    mHFile = NULL;
    mHFileMap = NULL;
    mHMapView = NULL;

    if (totalSize < mapViewSize || mapViewSize <= 0) {
        printf("size error\n");
        return false;
    }

    // ������ʱ�ļ���
    this->mPath.assign("");
    {
        char path[MAX_PATH] = { 0 };

        char tmpFileDir[MAX_PATH] = { 0 };
        GetTempPathA(MAX_PATH, tmpFileDir);

        char tmpFileName[L_tmpnam * 2] = { 0 };
        tmpnam(tmpFileName);

        snprintf(path, sizeof(path), "%s0epro_%stmp", tmpFileDir, tmpFileName + 1);
        this->mPath.assign(path);
    }

    do
    {
        /**�����ļ�*/
        mHFile = CreateFileA(this->mPath.data(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ /*| FILE_SHARE_WRITE*/,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (INVALID_HANDLE_VALUE == mHFile) {
            printf("CreateFile error!\n");
            break;
        }

        /**��ȡ�ļ���С*/
        LARGE_INTEGER fileSize;
        GetFileSizeEx(mHFile, &fileSize);
        printf("File size are %lld bytes and %4.2f KB and %4.2f MB and %4.2f GB\n",
            fileSize.QuadPart, /** bytes */
            (double)fileSize.QuadPart / (1024), /** KB */
            (double)fileSize.QuadPart / (1024 * 1024), /** MB */
            (double)fileSize.QuadPart / (1024 * 1024 * 1024)); /** GB */

        /**�����С��һ��, ���Ĵ�С*/
        if (fileSize.QuadPart != totalSize){
            fileSize.QuadPart = totalSize;
        }

        this->mFileMapName = GetGuidToString();

        /**�����ļ�ӳ���ں˶���*/
        mHFileMap = CreateFileMappingA(mHFile,
            NULL,
            PAGE_READWRITE,
            fileSize.HighPart,
            fileSize.LowPart,
            this->mFileMapName.data());
        if (mHFileMap != NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
            printf("CreateFileMapping error\n");
            break;
        }

        mTotalSize = totalSize;
        mMapViewSize = mapViewSize;

        return true; // �ɹ�
    } while (0);

    if (mHFileMap) {
        CloseHandle(mHFileMap);
        mHFileMap = NULL;
    }
    if (mHFile) {
        CloseHandle(mHFile);
        mHFile = NULL;
    }

    mFileMapName.assign("");

    return false;
}


bool LiteDataBaseWrite::DeInitWrite()
{
    if (mHMapView) {
        UnmapViewOfFile(mHMapView);
        mHMapView = NULL;
    }
    if (mHFileMap) {
        CloseHandle(mHFileMap);
        mHFileMap = NULL;
    }
    if (mHFile) {
        CloseHandle(mHFile);
        mHFile = NULL;
    }

    /*if (IsExists(this->mPath.data())) {
        DeleteFile(this->mPath.data());
        this->mPath.assign("");
    }*/

    return true;
}


bool LiteDataBaseWrite::WriteData(void* data, int len, LiteData* lite)
{
    if (NULL == lite || NULL == data || len <= 0) {
        printf("[%s %d] ��������\n", __FUNCTION__, __LINE__);
        return false;
    }

    int32_t addDataSize = sizeof(LiteHeader) + len; // ��� head �� data
    if (NULL == mHMapView || this->mCurrentOffset + addDataSize > this->mMapViewSize)
    {
        // ��ǰMapView���ڴ治������, ��Ҫ����
        if (false == mIsInCurrentMapView(mMapViewOffset + mCurrentOffset, addDataSize))
        {
            if (false == this->mUpdateMapView(mMapViewOffset + mCurrentOffset, addDataSize)) {
                return false;
            }
        }

        return this->WriteData(data, len, lite);
    }

    /** �������ݵ�λ�� */
    char* ptr = (char*)mHMapView + this->mCurrentOffset;

    /** ��Ϣͷ */
    LiteHeader head;
    memset(&head, 0, sizeof(LiteHeader));
    head.size = len;
    head.offset = mMapViewOffset + mCurrentOffset;
    memcpy(head.mark, "epro", sizeof(head.mark));

    __try{
        int offset = 0;
        memcpy(ptr + offset, &head, sizeof(LiteHeader)); offset += sizeof(LiteHeader);
        memcpy(ptr + offset, data, len);

        mCurrentOffset += addDataSize; /** ����MapView����ƫ�� */
    }
    __except (1) {
        printf("[%s %d]\n", __FUNCTION__, __LINE__);
        return false;
    }

    memset(lite, 0, sizeof(LiteData));
    lite->offset = head.offset;
    lite->size = head.size;

    printf("д����: [%10I64d, %10I64d] ���ƫ��: [%10d, %10d], ���ݳ���: %10d\n",
        lite->offset, lite->offset + addDataSize,
        mCurrentOffset - addDataSize, mCurrentOffset,
        len);

    return true;
}




bool LiteDataBaseWrite::mUpdateMapView(int64_t offset, int32_t addSize)
{
    offset = AlignUp_Int64(offset, SIZE_64KB);

    if (NULL != mHMapView) {
        /** ʣ�µĿռ䲻��addDataSize, ��ӳ����һ��mMapViewSize��С������û��ӳ�䵱ǰ�ռ���ǰ64KB */
        printf("\n�˷���: [%10I64d, %10I64d] (%I64d) bytes�ռ�\n\n", mMapViewOffset + mCurrentOffset, offset, offset - (mMapViewOffset + mCurrentOffset));
    }

    return LiteDataBase::mUpdateMapView(offset);
}

// LiteDataBaseRead

LiteDataBaseRead::LiteDataBaseRead()
{
    this->dwDesiredAccess = FILE_MAP_READ;
}


LiteDataBaseRead::~LiteDataBaseRead()
{
    this->DeInitReader();
}


bool LiteDataBaseRead::InitReader(std::string fileMapName, int64_t totalSize, int32_t mapViewSize)
{
    mHFileMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, TRUE, fileMapName.data());
    if (NULL == mHFileMap) {
        printf("open file map error!\n");
        return false;
    }

    mTotalSize = totalSize;
    mMapViewSize = mapViewSize;

    return true; // �ɹ�
}


bool LiteDataBaseRead::DeInitReader()
{
    if (mHMapView) {
        UnmapViewOfFile(mHMapView);
        mHMapView = NULL;
    }


    if (mHFileMap) {
        CloseHandle(mHFileMap);
        mHFileMap = NULL;
    }

    return true;
}


bool LiteDataBaseRead::ReadData(LiteData* lite, void* data, int len)
{
    if (NULL == lite || lite->offset < 0 || lite->size <= 0) {
        printf("[%s %d] ��������\n", __FUNCTION__, __LINE__);
        return false;
    }

    int32_t readSize = sizeof(int64_t)+sizeof(int32_t)+lite->size;
    if (false == mIsInCurrentMapView(lite->offset, readSize)) {
        if (false == mUpdateMapView(lite->offset)) {
            return false;
        }
    }

    /** �������ݵ�λ�� */
    char* ptr = (char*)this->mHMapView + (lite->offset - this->mMapViewOffset);

    /**��Ϣͷ*/
    LiteHeader head;
    memset(&head, 0, sizeof(LiteHeader));

    __try{
        memset(data, 0, len);

        int offset = 0;
        memcpy(&head, ptr + offset, sizeof(LiteHeader));  offset += sizeof(LiteHeader);
        if (head.size == lite->size &&
            head.offset == lite->offset &&
            memcmp(head.mark, "epro", sizeof(head.mark)) == 0) {
            memcpy(data, ptr + offset, lite->size);
        }
        else {
            printf("������\n");
            return false;
        }
    }
    __except (1) {
        printf("[%s %d]\n", __FUNCTION__, __LINE__);
        return false;
    }

    return true;
}

bool LiteDataBaseRead::mUpdateMapView(int64_t offset)
{
    offset = AlignDown_Int64(offset, SIZE_64KB);
    return LiteDataBase::mUpdateMapView(offset);
}
