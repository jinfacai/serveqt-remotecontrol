#include "Command.h"
#include "Packet.h"
#include <iostream>
#include "ServerSocket.h"

CCommand::CCommand() : m_serverSocket(nullptr) {
	struct { int nCmd; CMDFUNC func; }
	data[] = {
		{static_cast<int>(Type::TEXT_MESSAGE), &CCommand::handleTextMessage },
		{static_cast<int>(Type::FILE_START), &CCommand::handleFileStart },
		{static_cast<int>(Type::FILE_DATA), &CCommand::handleFileData },
		{static_cast<int>(Type::FILE_COMPLETE), &CCommand::handleFileComplete },
		{static_cast<int>(Type::TEST_CONNECT), &CCommand::handleTestConnect },
		{-1, nullptr}
	};
	for (int i = 0; data[i].nCmd != -1; ++i) {
		m_mapFunction.emplace(data[i].nCmd, data[i].func);
	}
}

int CCommand::ExecuteCommand(int nCmd, std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId) {
	auto it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}
	return(this->*(it->second))(lstPacket, packetQueue, inPacket, clientId);
}

// 客户端管理方法
void CCommand::addClient(int clientSocket, int clientId, const std::string& ip, int port) {
	m_clientManager.addClient(clientSocket, clientId, ip, port);
}

void CCommand::removeClient(int clientId) {
	m_clientManager.removeClient(clientId);
}

void CCommand::updateClientUsername(int clientId, const std::string& username) {
	m_clientManager.updateClientUsername(clientId, username);
}

// 数据包路由方法
void CCommand::broadcastPacket(const CPacket& packet, int excludeClientId) {
	if (!m_serverSocket) return;

	auto connectedIds = m_clientManager.getConnectedClientIds();
	for (int clientId : connectedIds) {
		if (clientId != excludeClientId) {
			m_serverSocket->sendPacketToClient(clientId, packet);
		}
	}
}

void CCommand::sendPacketToClient(int clientId, const CPacket& packet) {
	if (!m_serverSocket) return;

	if (m_clientManager.hasClient(clientId)) {
		m_serverSocket->sendPacketToClient(clientId, packet);
	}
}

void CCommand::sendSystemMessage(const std::string& message, int excludeClientId) {
	// 创建系统消息包
	CPacket systemPacket(static_cast<int>(Type::TEXT_MESSAGE), reinterpret_cast<const uint8_t*>(message.c_str()), message.size());
	broadcastPacket(systemPacket, excludeClientId);
}

// 处理聊天消息 - 支持同步和异步双重处理
int CCommand::handleTextMessage(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId) {
	std::string rawData = inPacket.getData();
	if (rawData.empty()) {
		std::cerr << "chat message content is empty" << std::endl;
		return -1;
	}

	// 添加发送者信息到消息中
	const ClientInfo* client = m_clientManager.getClient(clientId);
	if (client) {
		std::string senderInfo = "[" + std::to_string(clientId) + "] ";
		std::string fullMessage = senderInfo + rawData;

		// 创建新的消息包
		CPacket newPacket(static_cast<int>(Type::TEXT_MESSAGE), reinterpret_cast<const uint8_t*>(fullMessage.c_str()), fullMessage.size());

		// 同步处理：添加到lstPacket用于立即发送
		lstPacket.push_back(newPacket);

		// 异步处理：添加到packetQueue用于高并发场景
		packetQueue.push(static_cast<size_t>(Type::TEXT_MESSAGE), newPacket);
	}
	else {
		// 如果没有找到客户端信息，直接转发原包
		lstPacket.push_back(inPacket);
		packetQueue.push(static_cast<size_t>(Type::TEXT_MESSAGE), inPacket);
	}

	std::cout << "Forward chat messages from client " << clientId << ": " << rawData << std::endl;
	return 0;
}

// 文件首包 仅含filename - 支持同步和异步双重处理
int CCommand::handleFileStart(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId) {
	const std::string filename = inPacket.getData();
	if (filename.empty()) {
		std::cerr << "Command.cpp: " << "The file name is empty" << std::endl;
		return -1;
	}

	// 添加发送者信息
	const ClientInfo* client = m_clientManager.getClient(clientId);
	if (client) {
		std::string senderInfo = "[" + std::to_string(clientId) + "] ";
		std::string fullMessage = senderInfo + "started file transfer: " + filename;

		CPacket newPacket(static_cast<int>(Type::TEXT_MESSAGE), reinterpret_cast<const uint8_t*>(fullMessage.c_str()), fullMessage.size());

		// 同步处理：通知消息立即发送
		lstPacket.push_back(newPacket);

		// 异步处理：添加到packetQueue用于高并发场景
		packetQueue.push(static_cast<size_t>(Type::TEXT_MESSAGE), newPacket);
	}

	std::cout << "Client " << clientId << " started file transfer: " << filename << std::endl;

	// 同步处理：文件开始包立即发送
	lstPacket.push_back(inPacket);

	// 异步处理：添加到packetQueue用于高并发场景
	packetQueue.push(static_cast<size_t>(Type::FILE_START), inPacket);
	return 0;
}

// 中间数据 - 支持同步和异步双重处理
int CCommand::handleFileData(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId) {
	const std::string& chunk = inPacket.getData();
	std::cout << "Client " << clientId << " file data chunk size: " << chunk.size() << std::endl;

	// 同步处理：文件数据包立即发送
	lstPacket.push_back(inPacket);

	// 异步处理：添加到packetQueue用于高并发场景
	packetQueue.push(static_cast<size_t>(Type::FILE_DATA), inPacket);
	return 0;
}

int CCommand::handleFileComplete(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId) {
	const ClientInfo* client = m_clientManager.getClient(clientId);
	if (client) {
		std::string senderInfo = "[" + std::to_string(clientId) + "] ";
		std::string fullMessage = senderInfo + " file transfer completed";

		CPacket newPacket(static_cast<int>(Type::TEXT_MESSAGE), reinterpret_cast<const uint8_t*>(fullMessage.c_str()), fullMessage.size());

		// 同步处理：通知消息立即发送
		lstPacket.push_back(newPacket);

		// 异步处理：添加到packetQueue用于高并发场景
		packetQueue.push(static_cast<size_t>(Type::TEXT_MESSAGE), newPacket);
	}

	std::cout << "Client " << clientId << " file transfer completed" << std::endl;

	// 同步处理：文件完成包立即发送
	lstPacket.push_back(inPacket);

	// 异步处理：添加到packetQueue用于高并发场景
	packetQueue.push(static_cast<size_t>(Type::FILE_COMPLETE), inPacket);
	return 0;
}

// 处理测试连接 - 支持同步和异步双重处理
int CCommand::handleTestConnect(std::list<CPacket>& lstPacket, PacketQueue& packetQueue, CPacket& inPacket, int clientId) {
	// 返回简单的OK消息
	std::string okMsg = "OK";
	CPacket okPacket(static_cast<int>(Type::TEST_CONNECT),
		reinterpret_cast<const uint8_t*>(okMsg.data()), okMsg.size());

	// 同步处理：测试连接响应立即发送
	lstPacket.push_back(okPacket);

	// 异步处理：添加到packetQueue用于高并发场景
	packetQueue.push(static_cast<size_t>(Type::TEST_CONNECT), okPacket);

	std::cout << "Test connect successfully from client " << clientId << std::endl;
	return 0;
}

