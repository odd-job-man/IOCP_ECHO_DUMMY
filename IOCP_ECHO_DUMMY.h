#pragma once
#include "NetClient.h"
#include "StoredPacketInfo.h"

//struct StoredPacketInfo;
class IOCP_ECHO_DUMMY : public NetClient
{
public:
	bool Start();
	virtual void OnRecv(ULONGLONG id, Packet* pPacket) override;
	virtual void OnError(ULONGLONG id, int errorType, Packet* pRcvdPacket) override;
	virtual void OnConnect(ULONGLONG id) override;
	virtual void OnRelease(ULONGLONG id) override;
	virtual void OnConnectFailed(ULONGLONG id) override;
	virtual ~IOCP_ECHO_DUMMY();

	// ECHO 전용함수
	void SendPacket_OVERSEND(ULONGLONG id, Packet** ppPacketArr, ULONGLONG overSend);

	ULONGLONG overSend_ = 0;
	ULONGLONG PacketPayloadSize_ = 0;
	int RAND_DISCONNECT = 0;
private:
	void OverSend_COMMON(ULONGLONG id, StoredPacketInfo* pCurSPI, ULONGLONG lastSendNum);
};
