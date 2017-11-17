#pragma once
#include <stdint.h>
#include <Windows.h>
#include <iostream>
#include <string>

const int32_t SIZE_64KB = 1 * 64 * 1024; /** 64KB */

struct LiteData {
    int64_t offset; /**数据块相对FileMap的偏移*/
    int32_t size; /**数据大小, 校验*/
};

struct LiteHeader
{
    char mark[4]; /** 标识数据块的开始*/
    int32_t size; /** 数据部分长度 */
    int64_t offset; /** 数据块相对于FileMap的偏移 */
};

class LiteDataBase
{
public:
    LiteDataBase();
    virtual~LiteDataBase() { }
protected:
    /** 判断[offet, offset+size]是否在当前MapView中(offset: 相对FileMap的偏移) */
    bool mIsInCurrentMapView(int64_t offset, int32_t size = 0);

    /** 映射[offset, offset+mMapViewSize]之间的数据, offset为64KB(n*64*1024)的整数倍*/
    virtual bool mUpdateMapView(int64_t offset);

protected:
    HANDLE mHFileMap;  /** FileMap句柄 */
    void* mHMapView;  /** MapView句柄 */

    DWORD dwDesiredAccess; /** MapViewOfFile的权限*/

    int64_t mMapViewOffset; /** 当前的MapView相对FileMap的偏移*/
    int32_t mCurrentOffset; /** 访问数据位置相对MapView的偏移*/

    int64_t mTotalSize; /** FileMap总大小*/
    int32_t mMapViewSize; /** 每次取MapView数据的大小(可以变化, 这里固定了大小)*/
};

/** 写数据(一个) */
class LiteDataBaseWrite : public LiteDataBase
{
public:
    LiteDataBaseWrite();
    virtual ~LiteDataBaseWrite();

    /**返回FileMap名称, mapViewSize: 推荐以64*1024的整倍数*/
    bool InitWrite(int64_t totalSize, int32_t mapViewSize);
    bool DeInitWrite();

    /** FileMap名称 */
    std::string GetFileMapName() { return mFileMapName; }

    /** 写数据, lite输出 */
    bool WriteData(void* data, int len, LiteData* lite);

protected:
    /** 映射[offset, offset+mMapViewSize]之间的数据, offset会以64KB上取整*/
    bool mUpdateMapView(int64_t offset, int32_t addSize);

protected:
    HANDLE mHFile; /** 文件句柄 */
    std::string mPath; /** 文件路径 */
    std::string mFileMapName; /** FileMap名称 */
};

/** 读数据(多个) */
class LiteDataBaseRead : public LiteDataBase
{
public:
    LiteDataBaseRead();
    virtual ~LiteDataBaseRead();

    /** GetFileMapName() totalSize: 要和LiteDataBaseWrite的参数一致*/
    bool InitReader(std::string fileMapName, int64_t totalSize, int32_t mapViewSize);

    bool DeInitReader();

    /** 读数据 */
    bool ReadData(LiteData* lite, void* data, int len);

protected:
    /** 映射[offset, offset+mMapViewSize]之间的数据, offset会以64KB下取整*/
    virtual bool mUpdateMapView(int64_t offset) override;

};
