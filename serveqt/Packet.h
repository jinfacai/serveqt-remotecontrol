#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <string.h>
#include <iostream>

class CPacket
{
public:
    // 数据包结构：sHead sLength sCmd strData sSum
    // sLength = sizeof(sCmd) + sizeof(strData) + sizeof(sSum)

    // 默认构造函数
    CPacket();

    // 构造数据包
    CPacket(uint16_t nCmd, const uint8_t* pData, size_t nSize);

    // 解析数据包
    CPacket(const uint8_t* pData, size_t& nSize);

    // 拷贝构造函数
    CPacket(const CPacket& pack);

    // 析构函数
    ~CPacket();

    // 赋值运算符
    CPacket& operator=(const CPacket& pack);

    // 获取数据包大小
    int Size() const;

    // 获取数据包数据
    const char* Data() const;

    // 获取命令
    uint16_t getCmd() const { return sCmd; }

    // 获取数据
    const std::string& getData() const { return strData; }

    // 设置数据
    void setData(const std::string& data);

    // 设置命令
    void setCmd(uint16_t cmd);

private:
    uint16_t sHead;     // 2字节 - 包头标识 0xFEFF
    uint32_t sLength;   // 4字节 - 数据长度
    uint16_t sCmd;      // 2字节 - 命令
    uint16_t sSum;      // 2字节 - 校验和
    std::string strData; // 数据内容
    std::string strOut;  // 输出缓冲区

    // 计算校验和
    uint16_t calculateChecksum() const;

    // 验证数据包完整性
    bool validatePacket(const uint8_t* pData, size_t nSize) const;
};

#pragma pack(pop)