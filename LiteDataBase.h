#pragma once
#include <stdint.h>
#include <Windows.h>
#include <iostream>
#include <string>

const int32_t SIZE_64KB = 1 * 64 * 1024; /** 64KB */

struct LiteData {
    int64_t offset; /**���ݿ����FileMap��ƫ��*/
    int32_t size; /**���ݴ�С, У��*/
};

struct LiteHeader
{
    char mark[4]; /** ��ʶ���ݿ�Ŀ�ʼ*/
    int32_t size; /** ���ݲ��ֳ��� */
    int64_t offset; /** ���ݿ������FileMap��ƫ�� */
};

class LiteDataBase
{
public:
    LiteDataBase();
    virtual~LiteDataBase() { }
protected:
    /** �ж�[offet, offset+size]�Ƿ��ڵ�ǰMapView��(offset: ���FileMap��ƫ��) */
    bool mIsInCurrentMapView(int64_t offset, int32_t size = 0);

    /** ӳ��[offset, offset+mMapViewSize]֮�������, offsetΪ64KB(n*64*1024)��������*/
    virtual bool mUpdateMapView(int64_t offset);

protected:
    HANDLE mHFileMap;  /** FileMap��� */
    void* mHMapView;  /** MapView��� */

    DWORD dwDesiredAccess; /** MapViewOfFile��Ȩ��*/

    int64_t mMapViewOffset; /** ��ǰ��MapView���FileMap��ƫ��*/
    int32_t mCurrentOffset; /** ��������λ�����MapView��ƫ��*/

    int64_t mTotalSize; /** FileMap�ܴ�С*/
    int32_t mMapViewSize; /** ÿ��ȡMapView���ݵĴ�С(���Ա仯, ����̶��˴�С)*/
};

/** д����(һ��) */
class LiteDataBaseWrite : public LiteDataBase
{
public:
    LiteDataBaseWrite();
    virtual ~LiteDataBaseWrite();

    /**����FileMap����, mapViewSize: �Ƽ���64*1024��������*/
    bool InitWrite(int64_t totalSize, int32_t mapViewSize);
    bool DeInitWrite();

    /** FileMap���� */
    std::string GetFileMapName() { return mFileMapName; }

    /** д����, lite��� */
    bool WriteData(void* data, int len, LiteData* lite);

protected:
    /** ӳ��[offset, offset+mMapViewSize]֮�������, offset����64KB��ȡ��*/
    bool mUpdateMapView(int64_t offset, int32_t addSize);

protected:
    HANDLE mHFile; /** �ļ���� */
    std::string mPath; /** �ļ�·�� */
    std::string mFileMapName; /** FileMap���� */
};

/** ������(���) */
class LiteDataBaseRead : public LiteDataBase
{
public:
    LiteDataBaseRead();
    virtual ~LiteDataBaseRead();

    /** GetFileMapName() totalSize: Ҫ��LiteDataBaseWrite�Ĳ���һ��*/
    bool InitReader(std::string fileMapName, int64_t totalSize, int32_t mapViewSize);

    bool DeInitReader();

    /** ������ */
    bool ReadData(LiteData* lite, void* data, int len);

protected:
    /** ӳ��[offset, offset+mMapViewSize]֮�������, offset����64KB��ȡ��*/
    virtual bool mUpdateMapView(int64_t offset) override;

};
