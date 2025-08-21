#pragma once
#include <list>
#include <map>
#include <string>
#include <vector>
#include "ClientManager.h"
#include "CQueue.h"


class CPacket;
class CServerSocket;

class CCommand {
public:
	enum class Type : uint16_t {
		TEXT_MESSAGE = 1,      // ������Ϣ
		FILE_START = 2,        // �ļ��װ�
		FILE_DATA = 3,         // �ļ�����
		FILE_COMPLETE = 4,     // �ļ��������
		TEST_CONNECT = 1981    // ��������
	};

	// ����������� - ֧��ͬ��(lstPacket)���첽(packetQueue)˫�ش���
	using CMDFUNC = int (CCommand::*)(std::list<CPacket>&, PacketQueue&, CPacket&, int);

	CCommand();
	~CCommand() = default;

	// ִ�������������ID�������ҵ���߼�
	// ֧��ͬ��(lstPacket)���첽(packetQueue)���ִ���ʽ
	int ExecuteCommand(int nCmd, std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId = -1);

	// �ͻ��˹���
	void addClient(int clientSocket, int clientId, const std::string& ip, int port);
	void removeClient(int clientId);
	void updateClientUsername(int clientId, const std::string& username);

	// ��ȡ�ͻ�����Ϣ
	const ClientManager& getClientManager() const { return m_clientManager; }
	ClientManager& getClientManager() { return m_clientManager; }

	// ���ݰ�·��
	void setServerSocket(class CServerSocket* serverSocket) { m_serverSocket = serverSocket; }

private:
	std::map<int, CMDFUNC> m_mapFunction;
	ClientManager m_clientManager;                    // �ͻ��˹�����
	class CServerSocket* m_serverSocket;

	// ������� - ֧��ͬ�����첽˫�ش���
	int handleTextMessage(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId);
	int handleFileStart(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId);
	int handleFileData(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId);
	int handleFileComplete(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId);
	int handleTestConnect(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId);

	// ��������
	void broadcastPacket(const CPacket& packet, int excludeClientId = -1);
	void sendPacketToClient(int clientId, const CPacket& packet);
	void sendSystemMessage(const std::string& message, int excludeClientId = -1);
};

