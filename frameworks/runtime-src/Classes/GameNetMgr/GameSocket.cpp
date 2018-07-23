#include "GameSocket.h"
#include "cocos2d.h"
#include <sys/types.h>
#include <map>
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)

#else
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

using namespace cocos2d;
using namespace std;

static int _socket_index = 0;
static map<int, GameSocket*>_sockets;



#pragma mark DataBuff opt

int GameSocket::getDataBuff_in() 
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if (mutex_buff.try_lock())
	{
#else
	if (pthread_mutex_trylock(&mutex_buff) == 0) 
	{
#endif
		if (dataBuff_in.empty()) 
		{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
			mutex_buff.unlock();
#else
			pthread_mutex_unlock(&mutex_buff);
#endif
			return -1;
		}
		int pid = dataBuff_in.front();
		dataBuff_in.pop();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		mutex_buff.unlock();
#else
		pthread_mutex_unlock(&mutex_buff);
#endif
		return pid;
	}
	else
		return -1;
}

void GameSocket::putDataBuff_in(int pid) 
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	mutex_buff.lock();
#else
	pthread_mutex_lock(&mutex_buff);
#endif
	dataBuff_in.push(pid);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	mutex_buff.unlock();
#else
	pthread_mutex_unlock(&mutex_buff);
#endif
}

int GameSocket::getDataBuff_out() 
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if (mutex_buff.try_lock())
#else
	if (pthread_mutex_trylock(&mutex_buff) == 0)
#endif
	{
		if (dataBuff_out.empty()) 
		{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
			mutex_buff.unlock();
#else
			pthread_mutex_unlock(&mutex_buff);
#endif
			return -1;
		}
		int pid = dataBuff_out.front();
		dataBuff_out.pop();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		mutex_buff.unlock();
#else
		pthread_mutex_unlock(&mutex_buff);
#endif
		return pid;
	}
	else {
		return -1;
	}
}

void GameSocket::putDataBuff_out(int pid) 
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	mutex_buff.lock();
#else
	pthread_mutex_lock(&mutex_buff);
#endif
	dataBuff_out.push(pid);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	mutex_buff.unlock();
#else
	pthread_mutex_unlock(&mutex_buff);
#endif
}

int GameSocket::getDataBuff_in_size() 
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if (mutex_buff.try_lock())
#else
	if (pthread_mutex_trylock(&mutex_buff) == 0)
#endif
	{
		if (dataBuff_in.empty()) 
		{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
			mutex_buff.unlock();
#else
			pthread_mutex_unlock(&mutex_buff);
#endif
			return 0;
		}
		int size = dataBuff_in.size();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		mutex_buff.unlock();
#else
		pthread_mutex_unlock(&mutex_buff);
#endif
		return size;
	}
	else {
		return -1;
	}
}

int GameSocket::getDataBuff_out_size() 
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if (mutex_buff.try_lock())
#else
	if (pthread_mutex_trylock(&mutex_buff) == 0)
#endif
	{
		if (dataBuff_out.empty()) 
		{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
			mutex_buff.unlock();
#else
			pthread_mutex_unlock(&mutex_buff);
#endif
			return 0;
		}
		int size = dataBuff_out.size();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		mutex_buff.unlock();
#else
		pthread_mutex_unlock(&mutex_buff);
#endif
		return size;
	}
	else {
		return -1;
	}
}



GameSocket::GameSocket() {
	memset(m_buff_out, 0, sizeof(m_buff_out));
	memset(m_buff_in, 0, sizeof(m_buff_in));
	m_msg_offset = 4;
#if (CC_TARGET_PLATFORM != CC_PLATFORM_WIN32)
	mutex_buff = PTHREAD_MUTEX_INITIALIZER;
#endif
}


void GameSocket::getIPFromURL(char *ip, const char *url) 
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	[[NETWorker getInstance] checkIPV];
	if ([[NETWorker getInstance] isIPV6_NET]) {
		getIPFromURL_ipv6(ip, url);//����ios �� ipv6 ����
		return;
	}
#endif
	struct hostent* host;
	host = gethostbyname(url);
	if (host == nullptr) {
		perror("can not get host by url");
		return;
	}
	strcpy(ip, inet_ntoa(*((struct in_addr*)host->h_addr)));
}

int GameSocket::openSocket(const char *ip, int port) {//����һ��socket����ʼ��һЩ����
	CCLOG("Open port:%d and ip:%s", port, ip);
	GameSocket* so = new GameSocket();
	GameSocket::getIPFromURL(so->m_ip, ip);
	so->m_port = port;
	so->m_keepAlive = true;
	_socket_index++;
	so->m_sid = _socket_index;
	_sockets[_socket_index] = so;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	//����һ�����߳�
	std::thread p_thread((void*(*)(void*))&GameSocket::run, so);
	p_thread.detach();
#else
	//����һ�����߳�
	pthread_t th;
	//pthread_create(&th,NULL,,so);
	pthread_create(&th, NULL, (void*(*)(void*))&GameSocket::run, so);
	pthread_detach(th);
#endif
	return _socket_index;
}

void* GameSocket::run(void *obj) {

	GameSocket* so = (GameSocket*)obj;
	int ret = 0;
	do {
		if (!so->open()) {
			CCLOG("\n#################socket open error############\n");
			break;
		}

		while (!so->m_terminated) {
			if (so->check() == false) {
				CCLOG("\n#################socket check error############\n");
				break;//�������socket�Ƿ��������
			}
			ret = so->checkRead(0);//�������׽����Ƿ�ɶ�
			if (ret > 0) {
				bool ok = true;
				do {
					unsigned char buffer[_MAX_MSGSIZE] = { 0 };
					long size = sizeof(buffer);
					unsigned char* buff = buffer;
					if (so->receiveMsg(buff, size)) {
						if (size > 0) {
							int pid = DataBuff::createBuffer(buff, size, so->m_staticSN);
							so->putDataBuff_in(pid);
						}
					}
					else {
						ok = false;
					}
				} while (ok);
			}
			else if (ret < 0) {
				//so->writeError("CError0", "socket���ɶ�\n");
				CCLOG("\n#################socket can't read error############\n");
				break;
			}

			ret = so->checkWrite(1);//�������׽����Ƿ��д
			if (ret > 0) {
				int pid = so->getDataBuff_out();
				DataBuff* buff = DataBuff::getDataBuffer(pid);
				if (buff != nullptr) {
					//��ȡm_serialNumber

					unsigned char tmp = buff->_buff[10] ^ so->m_staticSN;
					int tmp_s = (int)(((buff->_buff[8] & 0xffffffff) << 24) + ((buff->_buff[9] & 0xffffffff) << 16) + ((tmp & 0xffffffff) << 8) + (buff->_buff[11] & 0xffffffff));
					/*
					char str[50] = {0};
					if (so->m_preSerialNumber != -1 && tmp_s-so->m_preSerialNumber != 1) {
					//������ssn
					sprintf(str, "���к��쳣sid = %d,serialNumber=%d,preSerialNumber=%d\n\n",so->m_sid,tmp_s,so->m_preSerialNumber);

					}else{
					//����
					sprintf(str, "sid = %d,serialNumber=%d\n\n",so->m_sid,tmp_s);
					}
					printf("%s",str);*/
					so->m_preSerialNumber = tmp_s;

					if (so->dealSendMsg(buff->_buff, buff->_length)) {
						so->sendNetMsg();
						DataBuff::deleteBuffer(pid);
					}
					else {
						CCLOG("\n#################socket write error############\n");
						break;
					}
				}
			}
			else if (ret < 0) {
				CCLOG("\n#################socket can't write error############\n");
				//so->writeError("CError0", "socket����д\n");
				break;
			}

		}

	} while (0);

	so->destroy();

	return nullptr;
}

bool GameSocket::open() 
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	if ([[NETWorker getInstance] isIPV6_NET]) 
	{
		return open_inIPV6();
	}
#endif
	m_socketStatu = 0;

	//ip��ַΪ�յ�ʱ��ֱ�ӷ���
	if (strcmp(m_ip, "") == 0) {
		CCLOG("##################################IP empty!#########################");
		return false;
	}

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == -1) {
		CCLOG("##################################socket create fail!#########################");
		return false;//����ʧ��
	}

	if (m_keepAlive) {
		/*int optval = 1;
		if (setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, sizeof(optval))) {
		return false;//����socket����ʧ��
		}*/
	}

	//int timeout = 2000;
	//setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(int));
	//setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));
	struct timeval timeout = { 3,0 };
	setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

	unsigned long serveraddr = inet_addr(m_ip);
	if (serveraddr == INADDR_NONE) {
		CCLOG("IP modality error:%s", m_ip);
		return false;//IP��ַ��ʽ����
	}

	sockaddr_in addr_in;
	memset((void*)&addr_in, 0, sizeof(addr_in));
	addr_in.sin_family = AF_INET;
	addr_in.sin_port = htons(m_port);
	addr_in.sin_addr.s_addr = serveraddr;

	if (connect(m_socket, (sockaddr*)&addr_in, sizeof(addr_in)) == -1) {
		CCLOG("Net connect fail:%s", m_ip);
		return false;//��������ʧ��
	}

	//����Ϊ������ģʽ
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	//����Ϊ������ģʽ
	unsigned long arg = 1;
	ioctlsocket(m_socket, FIONBIO, &arg);
#else
	//int flags = fcntl(m_socket, F_GETFL,0);
	//fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
	fcntl(m_socket, F_SETFL, O_NONBLOCK);
#endif
	m_buff_in_size = 0;
	m_buff_out_size = 0;
	m_buff_in_index = 0;

	/*
	struct linger so_linger;
	so_linger.l_onoff = 1;
	//so_linger.l_linger = 500;
	//���ر��׽��ֵ�ʱ�� �������������ݽ���ʱ�ر�
	setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (const char*)&so_linger, sizeof(so_linger));*/

	m_socketStatu = 1;

	CCLOG("socket open");
	return true;
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
//����IPV6 �¿���socket
bool GameSocket::open_inIPV6() {
	m_socketStatu = 0;

	if (strcmp(m_ip, "") == 0)
		return false;
	m_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

	if (m_socket == -1)
		return false;//����ʧ��

	struct timeval timeout = { 1,0 };
	setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
	setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));

	unsigned long serveraddr;
	in6_addr addr; // ipv6
	memset((void*)&addr, 0, sizeof(addr));
	serveraddr = inet_pton(AF_INET6, m_ip, &addr);
	if (serveraddr == INADDR_NONE) {
		CCLOG("----- INADDR-NONE ERRNO : %d", errno);
		return false;//IP��ַ��ʽ����
	}

	sockaddr_in6 addr_in;
	memset((void*)&addr_in, 0, sizeof(addr_in));
	addr_in.sin6_family = AF_INET6;
	addr_in.sin6_port = htons(m_port);
	addr_in.sin6_addr = addr;
	if (connect(m_socket, (sockaddr*)&addr_in, sizeof(addr_in)) == -1) {
		CCLOG("----- Connect ERRNO : %d", errno);
		return false;//��������ʧ��
	}
	fcntl(m_socket, F_SETFL, O_NONBLOCK);

	m_socketStatu = 1;

	return true;
}

// ipv6 �µ���������
void GameSocket::getIPFromURL_ipv6(char *ip, const char *url) {
	///////////////////
	struct addrinfo * result;
	struct addrinfo * res;
	char ipv4[128];
	char ipv6[128];
	int error;
	bool IS_IPV6 = false;
	bzero(&ipv4, sizeof(ipv4));
	bzero(&ipv4, sizeof(ipv6));
	CCLOG("GameSocket::getIPFromURL_ipv6  111111111");

	error = getaddrinfo(url, NULL, NULL, &result);
	if (error != 0) {
		CCLOG("error in getaddrinfo:%d", error);
		return;
	}
	for (res = result; res != NULL; res = res->ai_next) {
		char hostname[1025] = "";
		error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, 1025, NULL, 0, 0);
		if (error != 0) {
			CCLOG("error in getnameifno: %s", gai_strerror(error));
			continue;
		}
		else {
			switch (res->ai_addr->sa_family) {
			case AF_INET:
				memcpy(ipv4, hostname, 128);
				break;
			case AF_INET6:
				memcpy(ipv6, hostname, 128);
				IS_IPV6 = true;
			default:
				break;
			}
		}
	}
	CCLOG("GameSocket::getIPFromURL_ipv6  22222222");
	freeaddrinfo(result);

	if (IS_IPV6 == true) {
		strcpy(ip, ipv6);//�޸Ķ����� ip ����������Ϊ64 ,
	}
	else {
		strcpy(ip, ipv4);
	}
	////////////////
}

#endif

int GameSocket::getStatu() {
	return m_socketStatu;
}

int GameSocket::getSerialNumber() {
	if (m_serialNumber == -1) {
		return -1;
	}
	++m_serialNumber;
	return m_serialNumber;
}

int GameSocket::getStaticSerialNumber() {
	return m_serialNumber;
}

bool GameSocket::check() {
	if (m_socket == -1) {
		writeError("CError0", "socket��ʧ\n");
		return false;
	}
	char buf[1];
	int ret = recv(m_socket, buf, 1, MSG_PEEK);//���ĸ�������ΪMSG_PEEKΪ�鿴��ǰ���ݵ����������������Լ��Ӷ�����ɾ��
	if (ret == 0) {
		//socket no reponse
		writeError("CError0", "�Զ�û����Ӧ\n");
		CCLOG("close no reponse");
		return false;
	}
	else if (ret < 0) {
		if (hasError()) {
			//socket error
			writeError("CError0", "socket�ڲ��жϴ���\n");
			return false;
		}
		else {
			//������
			return true;
		}
	}
	else {//������
		return true;
	}

	return true;
}

int GameSocket::checkRead(int s) {
	fd_set rdfds;
	struct timeval tmTimeOut;

	if (m_socket <= 0) {
		return -1;
	}
	FD_ZERO(&rdfds);
	FD_SET(m_socket, &rdfds);

	tmTimeOut.tv_sec = s;
	tmTimeOut.tv_usec = 10000;
	return select(m_socket + 1, &rdfds, (fd_set*)0, (fd_set*)0, &tmTimeOut);
}

bool GameSocket::receiveMsg(void *buff, long &size) {
	if (buff == NULL || size <= 0) {
		return false;//not data
	}

	if (m_socket == -1) {
		return false;
	}

	if (m_buff_in_size < MSGLENBIT) {
		if (!readMSG() || m_buff_in_size < MSGLENBIT) {
			return false;
		}
	}
	//��ȡ���ݰ���ʵ�ʳ���
	int _msg_size = (int)(((m_buff_in[0] & 0xff) << 24) + ((m_buff_in[1] & 0xff) << 16) + ((m_buff_in[2] & 0xff) << 8) + (m_buff_in[3] & 0xff));
	//int _msg_size = (int) (((m_buff_in[4] & 0xffff) << 8) + ((m_buff_in[5] & 0xffff)));
	_msg_size = _msg_size + m_msg_offset;

	//short _pro = (short) (((m_buff_in[4] & 0xff) << 8) + (m_buff_in[5] & 0xff));//Э���
	if (m_serialNumber == -1) {
		//printf("����ǰ");
		for (int k = 4; k<_msg_size; k++)
		{
			if ((k - 4) % 2 == 0) {
				m_buff_in[k] ^= (_msg_size - 4);
			}
		}
		m_serialNumber = (int)(((m_buff_in[8] & 0xff) << 24) + ((m_buff_in[9] & 0xff) << 16) + ((m_buff_in[10] & 0xff) << 8) + (m_buff_in[11] & 0xff));
		m_staticSN = m_serialNumber;
		size = 0;
		m_buff_in_size = 0;
		return true;
	}
	/*
	if (_msg_size == 10) {
	size = 0;
	m_buff_in_size = 0;
	return true;
	}
	*/
    log("GameSocket::receiveMsg %d,%d,%d>>", _msg_size, m_buff_in_size, INBUFFSIZE);
	if (_msg_size <= 0 || _msg_size > INBUFFSIZE) {
		m_buff_in_size = 0;
		m_buff_in_index = 0;
		_msg_size = 0;
		return false;
	}
	if (_msg_size > m_buff_in_size) {

		if (!readMSG() || _msg_size > m_buff_in_size) {
			return false;
		}
	}

	memcpy(buff, m_buff_in, _msg_size);
	size = _msg_size;

	if (m_buff_in_size == _msg_size) {
		//������Ϣ���������
		m_buff_in_size = 0;
	}
	else {
		//��һ��������Ϣ   ����ճ��
		memcpy(m_buff_in, m_buff_in + _msg_size, m_buff_in_size - _msg_size);
		m_buff_in_size = m_buff_in_size - _msg_size;
	}

	return true;
}

bool GameSocket::readMSG() {
	//�жϻ᲻�ᳬ��Լ���ĳ���
	if (m_buff_in_size >= INBUFFSIZE || m_socket == -1) {
		return false;
	}
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	ssize_t inlen = recv(m_socket, (char *)m_buff_in + m_buff_in_size, INBUFFSIZE - m_buff_in_size, 0);
#else
	ssize_t inlen = recv(m_socket, m_buff_in + m_buff_in_size, INBUFFSIZE - m_buff_in_size, 0);
#endif
	if (inlen > 0) {
		m_buff_in_size += inlen;
		if (m_buff_in_size > INBUFFSIZE) {
			return false;
		}
		return true;
	}
	return false;
}

int GameSocket::checkWrite(int s) {
	fd_set wrdfds;
	struct timeval tmTimeOut;
	if (m_socket <= 0) {
		return -1;
	}

	FD_ZERO(&wrdfds);
	FD_SET(m_socket, &wrdfds);

	tmTimeOut.tv_sec = s;
	tmTimeOut.tv_usec = 10000;

	return select(m_socket + 1, (fd_set*)0, &wrdfds, (fd_set*)0, &tmTimeOut);
}

bool GameSocket::dealSendMsg(void *buff, long size) {
	if (buff == NULL || size <= 0) {
		writeError("CError0", "buffΪ��\n");
		return false;
	}

	if (m_socket == -1) {

		writeError("CError0", "socket��ʧ\n");

		return false;
	}

	if (size > OUTBUFFSIZE) {
		writeError("CError0", "�������ݴ���outbuffsize\n");

		return false;
	}

	if (m_buff_out_size + size > OUTBUFFSIZE) {
		if (!sendNetMsg()) {

			writeError("CError0", "��������ʧ��\n");

			return false;
		}
	}

	memcpy(m_buff_out_size + m_buff_out, buff, size);
	m_buff_out_size += size;
	return true;
}

bool GameSocket::sendNetMsg() {
	if (m_socket == -1) {
		return false;
	}

	if (m_buff_out_size <= 0) {
		return false;
	}

	int outsize = 0;
	//��ȫ����Ϣ�������������ѭ��
	do {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		int send_sz = send(m_socket, (char *)m_buff_out + outsize, m_buff_out_size - outsize, 0);
#else
		int send_sz = send(m_socket, m_buff_out + outsize, m_buff_out_size - outsize, 0);
#endif
		if (send_sz > 0) {
			outsize += send_sz;
		}
		else {
			if (hasError()) {
				return false;
			}
		}
	} while (m_buff_out_size > outsize);

	m_buff_out_size = 0;
	return true;
}

void GameSocket::testSend(void* buff, int size) {
	if (m_socket == -1) {
		return;
	}
	if (size < 0) {
		return;
	}
	//int s = send(m_socket, buff, size, 0);
	//printf("s = %d", s);
}

bool GameSocket::hasError() {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	int err = WSAGetLastError();
	if (err != WSAEWOULDBLOCK)
	{
		CCLOG("\nLast error code:%d\n", err);
		return true;
	}
#else
	int err = errno;

	if (err != EINPROGRESS && err != EAGAIN) 
	{
		CCLOG("\nLast error code:%d\n", err);
		return true;
	}
#endif
	return false;
}

void GameSocket::destroy() {
	m_socketStatu = -1;

	if (m_socket > 0) {
		/*struct linger so_linger;
		so_linger.l_onoff = 1;
		so_linger.l_linger = 100;
		setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (const char*)&so_linger, sizeof(so_linger));*/
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		closesocket(m_socket);
#else
		close(m_socket);
#endif
		m_socket = -1;
		_sockets[m_sid] = nullptr;
	}

	delete  this;

}

void GameSocket::closeSocket(int sid)
{
	GameSocket* so = _sockets[sid];
	if (so == nullptr) {
		return;
	}
	so->m_terminated = true;
	//delete so;
	//so = nullptr;
	_sockets[sid] = nullptr;
}

GameSocket* GameSocket::getSocket(int sid) {
	//printf("sid = %d",sid);
	return _sockets[sid];
}

void GameSocket::writeError(const char* filename, const char* content) {
	string ss = FileUtils::getInstance()->getWritablePath() + filename;//FileUtils::getInstance()->fullPathForFilename(filename);
	long len = strlen(content);
	FILE *fp = fopen(ss.c_str(), "a+");
	if (fp == NULL) {
		fp = fopen(ss.c_str(), "wb");
	}
	if (fp == NULL) {
		return;
	}
	fwrite(content, len, 1, fp);
	fflush(fp);
	fclose(fp);
}

GameSocket::~GameSocket() {
    _sockets[m_sid] = nullptr;
}
