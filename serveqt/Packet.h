#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <string.h>
#include <iostream>

class CPacket
{
public:
    // ���ݰ��ṹ��sHead sLength sCmd strData sSum
    // sLength = sizeof(sCmd) + sizeof(strData) + sizeof(sSum)

    // Ĭ�Ϲ��캯��
    CPacket();

    // �������ݰ�
    CPacket(uint16_t nCmd, const uint8_t* pData, size_t nSize);

    // �������ݰ�
    CPacket(const uint8_t* pData, size_t& nSize);

    // �������캯��
    CPacket(const CPacket& pack);

    // ��������
    ~CPacket();

    // ��ֵ�����
    CPacket& operator=(const CPacket& pack);

    // ��ȡ���ݰ���С
    int Size() const;

    // ��ȡ���ݰ�����
    const char* Data() const;

    // ��ȡ����
    uint16_t getCmd() const { return sCmd; }

    // ��ȡ����
    const std::string& getData() const { return strData; }

    // ��������
    void setData(const std::string& data);

    // ��������
    void setCmd(uint16_t cmd);
#pragma pack(1)

private:
    uint16_t sHead;     // 2�ֽ� - ��ͷ��ʶ 0xFEFF
    uint32_t sLength;   // 4�ֽ� - ���ݳ���
    uint16_t sCmd;      // 2�ֽ� - ����
    uint16_t sSum;      // 2�ֽ� - У���
    std::string strData; // ��������
    std::string strOut;  // ���������

    // ����У���
    uint16_t calculateChecksum() const;

    // ��֤���ݰ�������
    bool validatePacket(const uint8_t* pData, size_t nSize) const;
};

#pragma pack(pop)