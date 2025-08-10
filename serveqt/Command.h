#pragma once
#include <list>
#include <map>
#include <string>
#include <vector>
#include "ClientManager.h"


class CPacket;
class CServerSocket;

class CCommand {
public:
	enum class Type : uint16_t {
		TEXT_MESSAGE = 1,      // 聊天消息
		FILE_START = 2,        // 文件首包
		FILE_DATA = 3,         // 文件数据
		FILE_COMPLETE = 4,     // 文件传输完成
		TEST_CONNECT = 1981    // 测试连接
	};

	// 命令处理函数类型
	using CMDFUNC = int (CCommand::*)(std::list<CPacket>&, CPacket&, int);

	CCommand();
	~CCommand() = default;

	// 执行命令：根据命令ID处理具体业务逻辑
	int ExecuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket, int clientId = -1);

	// 客户端管理
	void addClient(int clientSocket, int clientId, const std::string& ip, int port);
	void removeClient(int clientId);
	void updateClientUsername(int clientId, const std::string& username);

	// 获取客户端信息
	const ClientManager& getClientManager() const { return m_clientManager; }
	ClientManager& getClientManager() { return m_clientManager; }

	// 数据包路由
	void setServerSocket(class CServerSocket* serverSocket) { m_serverSocket = serverSocket; }

private:
	std::map<int, CMDFUNC> m_mapFunction;
	ClientManager m_clientManager;                    // 客户端管理器
	class CServerSocket* m_serverSocket;

	// 命令处理器
	int handleTextMessage(std::list<CPacket>& lstPacket, CPacket& inPacket, int clientId);
	int handleFileStart(std::list<CPacket>& lstPacket, CPacket& inPacket, int clientId);
	int handleFileData(std::list<CPacket>& lstPacket, CPacket& inPacket, int clientId);
	int handleFileComplete(std::list<CPacket>& lstPacket, CPacket& inPacket, int clientId);
	int handleTestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket, int clientId);

	// 辅助方法
	void broadcastPacket(const CPacket& packet, int excludeClientId = -1);
	void sendPacketToClient(int clientId, const CPacket& packet);
	void sendSystemMessage(const std::string& message, int excludeClientId = -1);
};

