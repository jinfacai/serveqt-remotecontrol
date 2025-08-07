#pragma once
#include"Packet.h"
#include <sys/socket.h>
#include <iostream>
#include <map>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>


#define MAX_EVENTS 1024 
#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define DEFAULT_PORT 8080

struct ClientInfo {
	int socket;
	int id;
	std::string ip;
	int port;
	std::string receiverBuffer;
	bool isConnected;

	ClientInfo() : socket(-1),id(-1),ip(""), port(0), isConnected(false) {}

	ClientInfo(const ClientInfo& other)
		: socket(other.socket),id(other.id),ip(other.ip),
		port(other.port),receiverBuffer(other.receiverBuffer),
		isConnected(other.isConnected){ }

	ClientInfo& operator = (const ClientInfo& other) {
		if (this != &other) {
			socket = other.socket;
			id = other.id;
			ip = other.ip;
			port = other.port;
			receiverBuffer = other.receiverBuffer;
			isConnected = other.isConnected;
		}
		return *this;
	}
};

class CServerSocket
{
public:

	CServerSocket(const std::string& ip = "127.0.0.1", int port = DEFAULT_PORT);
	~CServerSocket();

	//启动服务器
	bool start();

	//停止服务器
	void stop();

	//运行服务器主循环
	void run();

	//检查服务器是否运行
	bool isRunning() const {
		return m_running;
	}

	//获取连接的客户端数量
	int getClientCount() const {
		return m_clients.size();
	}
	
	//广播消息给所有客户端
	void broadcastPacket(const CPacket& packet);

	//发送消息给指定客户端
	bool sendPacketToClient(int clientId, const CPacket& packet);

private:
	bool m_running;
	std::map<int, ClientInfo> m_clients;
	std::map<int, int>m_clientIdMap;
	int m_listenFd;
	int m_epollFd;
	int m_port;
	int m_nextClientId;
	std::string m_ip;
	
	//初始化服务器
	bool initialize();

	//打印日志信息
	void log(const std::string message) const;

	//设置非阻塞
	void setNonBlocking(int fd);

	//处理客户端新连接
	void handleNewConnection();

	//添加客户端到epoll
	void addClientToEpoll(int clientSocket);

	//处理客户端数据
	void handleClientData(int clientSocket);

	// 处理客户端断开
	void handleClientDisconnect(int clientSocket);

	// 从epoll移除客户端
	void removeClientFromEpoll(int clientSocket);

	//处理数据包
	void handlePacket(int clientSocket, const CPacket& packet);

	// 创建响应数据包
	CPacket createResponsePacket(uint16_t cmd, const std::string& data);

};

