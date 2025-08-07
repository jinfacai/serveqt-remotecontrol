#include "Packet.h"
#include <arpa/inet.h>

CPacket::CPacket() : sHead(0), sLength(0), sCmd(0), sSum(0)
{
}

CPacket::CPacket(uint16_t nCmd, const uint8_t* pData, size_t nSize)
{
    // nSize = strData.size()
    sHead = 0xFEFF; // 0xFF 0xFE
    sLength = nSize + 2 + 2; // sizeof(sCmd) + sizeof(sSum)
    sCmd = nCmd;

    if (nSize > 0) {
        strData.resize(nSize);
        memcpy((void*)strData.data(), pData, nSize); // pData -> strData
    }
    else {
        // ��¼������־��ʹ�ñ�׼������־���������⣩
        std::cerr << "Error: Received packet with zero data size (Cmd=" << sCmd << ")" << std::endl;
        strData.clear();
    }

    sSum = calculateChecksum();
}

CPacket::CPacket(const uint8_t* pData, size_t& nSize)
{
    // nSize = pack.size
    size_t i = 0;

    // ���Ұ�ͷ
    for (; i < nSize; i++) {
        if (*(uint16_t*)(pData + i) == 0xFEFF) {
            sHead = *(uint16_t*)(pData + i);
            i += 2;
            break;
        }
    }

    // ���ʣ�������Ƿ��㹻��sLength 4�ֽ� + sCmd 2�ֽ� + sSum 2�ֽڣ�
    if (i + 4 + 2 + 2 > nSize) {
        std::cerr << "Error: Incomplete packet header (need "
            << (i + 8) << " bytes, available " << nSize << ")" << std::endl;
        nSize = 0;
        return;
    }

    // ��ȡ���ݳ���
   // sLength = *(uint32_t*)(pData + i);
    sLength = ntohl(*(uint32_t*)(pData + i));
    i += 4;

    // ���δ��ȫ���յ����ݰ�������ʧ��
    if (sLength + i > nSize) {
        std::cerr << "Error: Incomplete packet (need "
            << (sLength + i) << " bytes, available " << nSize << ")" << std::endl;
        nSize = 0;
        return;
    }

    // ��ȡ����
    //sCmd = *(uint16_t*)(pData + i);
    sCmd = ntohs(*(uint16_t*)(pData + i));
    i += 2;

    // strData���ݶ�ȡ���д���
    if (sLength > 4) {
        strData.resize(sLength - 2 - 2); // sLength - 2 - 2 = sizeof(strData)
        memcpy((void*)strData.data(), pData + i, sLength - 2 - 2);
        i += sLength - 2 - 2;
    }

    // У��У���
    //sSum = *(uint16_t*)(pData + i);
    sSum = ntohs(*(uint16_t*)(pData + i));
    uint16_t sum = calculateChecksum();

    if (sum == sSum) {
        nSize -= i; // �Ƴ�ճ��
        // �޸�Ϊʣ��δ��������
        return;
    }
    else {
        std::cerr << "Error: Checksum mismatch" << std::endl;
        nSize = 0;
    }
}

CPacket::CPacket(const CPacket& pack)
{
    sHead = pack.sHead;
    sLength = pack.sLength;
    sCmd = pack.sCmd;
    strData = pack.strData;
    sSum = pack.sSum;
}

CPacket::~CPacket()
{
}

CPacket& CPacket::operator=(const CPacket& pack)
{
    if (this != &pack) {  // ��ֹ�Ը�ֵ
        sHead = pack.sHead;
        sLength = pack.sLength;
        sCmd = pack.sCmd;
        strData = pack.strData; // ���
        sSum = pack.sSum;
    }
    return *this;  // ���ض�������
}

int CPacket::Size() const
{
    return sLength + 6; // ���ݰ��ܴ�С
}

const char* CPacket::Data() const
{
    //strOut.resize(Size());
    std::string& mutableStrOut = const_cast<std::string&>(strOut);
    //uint8_t* pData = reinterpret_cast<uint8_t*>(&strOut[0]);
    mutableStrOut.resize(Size());
    uint8_t* pData = reinterpret_cast<uint8_t*>(&mutableStrOut[0]);

    *reinterpret_cast<uint16_t*>(pData) = sHead;
    pData += 2;

    *reinterpret_cast<uint32_t*>(pData) = htonl(sLength);
    pData += 4;

    *reinterpret_cast<uint16_t*>(pData) = htons(sCmd);
    pData += 2;

    memcpy(pData, strData.data(), strData.size());
    pData += strData.size();

    *reinterpret_cast<uint16_t*>(pData) = htons(sSum);

    return  mutableStrOut.c_str();
}

void CPacket::setData(const std::string& data)
{
    strData = data;
    sLength = data.size() + 2 + 2; // sizeof(sCmd) + sizeof(sSum)
    sSum = calculateChecksum();
}

void CPacket::setCmd(uint16_t cmd)
{
    sCmd = cmd;
    sSum = calculateChecksum();
}

uint16_t CPacket::calculateChecksum() const
{
    uint16_t sum = 0;
    for (uint8_t byte : strData) {
        sum += byte;
    }
    return sum;
}

bool CPacket::validatePacket(const uint8_t* pData, size_t nSize) const
{
    if (nSize < 8) { // ��С����С��2(ͷ) + 4(����) + 2(����) + 2(У���)
        return false;
    }

    // ����ͷ
    if (*(uint16_t*)pData != 0xFEFF) {
        return false;
    }

    // ������ݳ���
    uint32_t length = *(uint32_t*)(pData + 2);
    if (length + 6 > nSize) { // 6 = 2(ͷ) + 4(����)
        return false;
    }

    return true;
}