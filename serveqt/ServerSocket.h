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

	//����������
	bool start();

	//ֹͣ������
	void stop();

	//���з�������ѭ��
	void run();

	//���������Ƿ�����
	bool isRunning() const {
		return m_running;
	}

	//��ȡ���ӵĿͻ�������
	int getClientCount() const {
		return m_clients.size();
	}
	
	//�㲥��Ϣ�����пͻ���
	void broadcastPacket(const CPacket& packet);

	//������Ϣ��ָ���ͻ���
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
	
	//��ʼ��������
	bool initialize();

	//��ӡ��־��Ϣ
	void log(const std::string message) const;

	//���÷�����
	void setNonBlocking(int fd);

	//����ͻ���������
	void handleNewConnection();

	//��ӿͻ��˵�epoll
	void addClientToEpoll(int clientSocket);

	//����ͻ�������
	void handleClientData(int clientSocket);

	// ����ͻ��˶Ͽ�
	void handleClientDisconnect(int clientSocket);

	// ��epoll�Ƴ��ͻ���
	void removeClientFromEpoll(int clientSocket);

	//�������ݰ�
	void handlePacket(int clientSocket, const CPacket& packet);

	// ������Ӧ���ݰ�
	CPacket createResponsePacket(uint16_t cmd, const std::string& data);

};

