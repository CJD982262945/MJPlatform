#ifndef GameSocket_h
#define GameSocket_h
#pragma once //��֤ͷ�ļ�ֻ������һ��
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "DataBuff.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
#import "NETWorker.h"
#endif

#define _MAX_MSGSIZE 32*1024//16 * 1024
#define INBUFFSIZE   32*1024//(64*1024)
#define OUTBUFFSIZE  (5*1024)
#define MSGLENBIT 4
//��Ϣ�ṹ  ���峤��4�ֽ�  ��ͷ2�ֽ�  ���к�4�ֽ� + ����
class GameSocket {
public:
	char m_ip[32];
	int m_port;
	int m_socket;//socket
	int m_keepAlive;//�Ƿ񱣳ֻ�Ծ
	int m_socketStatu = 0; // 0 �ȴ����� 1 ���ӳɹ� -1 �ر�
	bool m_terminated = false;//���socket�Ƿ��Ѿ�����ֹ
	int m_sid = -1;//��¼�Լ��ı��
	int m_serialNumber = -1;//Ĭ�����к�Ϊ-1 ����û�н��յ������к�
	int m_staticSN = -1;
	int m_preSerialNumber = -1;

	int m_msg_offset;//ÿ���׽��ֵ�ƫ����
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	std::mutex mutex_buff;//������
#else
	pthread_mutex_t mutex_buff;//������
#endif
	unsigned char m_buff_out[OUTBUFFSIZE];
	unsigned char m_buff_in[INBUFFSIZE];
	int m_buff_out_size;
	int m_buff_in_size;
	int m_buff_in_index;

	GameSocket();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	//IPV6���
	bool	open_inIPV6();
	static void getIPFromURL_ipv6(char* ip, const char* url);
#endif

	queue<int> dataBuff_in;
	queue<int> dataBuff_out;


	int getDataBuff_in();
	void putDataBuff_in(int pid);
	int getDataBuff_out();
	void putDataBuff_out(int pid);
	int getDataBuff_in_size();
	int getDataBuff_out_size();

	static int openSocket(const char* ip, int port);//����һ��socket
	static void getIPFromURL(char* ip, const char* url);
	static void* run(void* obj);
	bool	open();
	int     getStatu();
	int     getSerialNumber();
	int     getStaticSerialNumber();
	bool check();
	int checkRead(int s);
	bool receiveMsg(void* buff, long& size);
	bool readMSG();
	int checkWrite(int s);
	bool dealSendMsg(void* buff, long size);
	bool sendNetMsg();
	void testSend(void* buff, int size);
	bool hasError();
	void destroy();

	static void    closeSocket(int sid);
	static GameSocket* getSocket(int sid);
	void writeError(const char* filename, const char* content);

	~GameSocket();
};


#endif /* GameSocket */
