#include "LiteDataBase.h"
#include <list>

struct MyStruct
{
    int age;
    char name[64];
    char desc[256];
};

int main(int argc, char* argv[])
{
    LiteDataBaseWrite writeLite;
    LiteDataBaseRead readLite;

    int64_t totalSize = 500 * 1024 * 1024;
    int32_t mapViewSize = 64 * 1024+1;

    writeLite.InitWrite(totalSize, mapViewSize);
    readLite.InitReader(writeLite.GetFileMapName(), totalSize, mapViewSize);

    std::list<LiteData> dataList;
    MyStruct st;
    for (int i = 0; i < 1000; i++)
    {
        memset(&st, 0, sizeof(MyStruct));
        st.age = i;
        _snprintf(st.name, sizeof(st.name), "(name %5d)", st.age);
        _snprintf(st.desc, sizeof(st.desc), "my name: %s, age: %5d", st.name, st.age);

        printf("%s\n", st.name);

        LiteData lite;
        if (writeLite.WriteData(&st, sizeof(MyStruct), &lite)) {
            dataList.push_back(lite);
        }
    }

    for (auto iter = dataList.begin(); iter != dataList.end(); iter++)
    {
        LiteData lite = *iter;
        memset(&st, 0, sizeof(MyStruct));
        if (readLite.ReadData(&lite, &st, sizeof(MyStruct))) {
            //printf("name: %s  age: %d  desc: %s\n", st.name, st.age, st.desc);
        }
    }

    system("pause");

    readLite.DeInitReader();
    writeLite.DeInitWrite();

    return 0;
}






















#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <io.h>
#include <stdint.h>
bool IsExists(const char* path)
{
    if (_access(path, 0) != -1) {
        return true;
    }

    return false;
}

// http://www.cnblogs.com/5iedu/p/4899791.html
int main2(int argc, char* argv[])
{
    {
        char path[MAX_PATH] = { 0 };
        char tmpFileDir[MAX_PATH] = { 0 };
        GetTempPathA(MAX_PATH, tmpFileDir);

        char tmpFileName[L_tmpnam * 2] = { 0 };
        tmpnam(tmpFileName);

        _snprintf(path, sizeof(path), "%slitefile_%stmp", tmpFileDir, tmpFileName + 1);
        printf("");
    }

    char* path = "G:/ZJ.dat";
    bool is_exists = IsExists(path);
    DWORD dwCreationDisposition = CREATE_ALWAYS;
    if (is_exists) {
        dwCreationDisposition = OPEN_EXISTING;
    }

    HANDLE hFile = NULL;
    HANDLE hFileMap = NULL;
    void* hMapView = NULL;

    do
    {
        hFile = CreateFileA(path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ /*| FILE_SHARE_WRITE*/,
            NULL,
            dwCreationDisposition,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (INVALID_HANDLE_VALUE == hFile) {
            printf("CreateFile error!\n");
            break;
        }

        LARGE_INTEGER fileSize;
        GetFileSizeEx(hFile, &fileSize);
        printf("File size are %lld bytes and %4.2f KB and %4.2f MB and %4.2f GB\n",
            fileSize.QuadPart, /** bytes */
            (double)fileSize.QuadPart / (1024), /** KB */
            (double)fileSize.QuadPart / (1024 * 1024), /** MB */
            (double)fileSize.QuadPart / (1024 * 1024 * 1024)); /** GB */

        hFileMap = CreateFileMappingA(hFile,
            NULL,
            PAGE_READWRITE,
            0,
            500 * 1024 * 1024, /** 比文件大小大，会追加 */
            "ZJ");
        if (hFileMap != NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
            printf("CreateFileMapping error\n");
            break;
        }

        __int64 totalSize = fileSize.QuadPart;
        __int64 offset = 0;
        int size = 64 * 1024;
        while (true)
        {
            hMapView = MapViewOfFile(hFileMap,
                FILE_MAP_ALL_ACCESS,
                0,
                500 * 1024 * 1024 - 64 * 1024, /** 即为64KB的整数倍 */
                15);
            if (NULL == hMapView) {
                printf("MapViewOfFile error! %d\n", GetLastError());
                break;
            }

            if (hMapView) {
                UnmapViewOfFile(hMapView);
                hMapView = NULL;
            }

            offset += 64 * 1024;

        }

    } while (0);

    if (hMapView) {
        UnmapViewOfFile(hMapView);
    }
    if (hFileMap) {
        CloseHandle(hFileMap);
    }
    if (hFile) {
        CloseHandle(hFile);
    }

    return 0;
}
