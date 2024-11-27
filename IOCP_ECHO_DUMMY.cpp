#include <new>
#include "IOCP_ECHO_DUMMY.h"
#include "Parser.h"
#include "MemLog.h"

#define LOGIN_PAYLOAD ((ULONGLONG)0x7fffffffffffffff)


StoredPacketInfo* g_pSPI;

bool IOCP_ECHO_DUMMY::Start()
{
	char* pStart;
	PARSER psr = CreateParser(L"ClientConfig.txt");

	GetValue(psr, L"OVERSEND", (PVOID*)&pStart, nullptr);
	overSend_ = (ULONGLONG)_wtoi((LPCWSTR)pStart);
	StoredPacketInfo::overSend = overSend_;

	GetValue(psr, L"PACKET_PAYLOAD_SIZE", (PVOID*)&pStart, nullptr);
	PacketPayloadSize_ = (ULONGLONG)_wtoi((LPCWSTR)pStart);

	GetValue(psr, L"RAND_DISCONNECT", (PVOID*)&pStart, nullptr);
	RAND_DISCONNECT = (ULONGLONG)_wtoi((LPCWSTR)pStart);
	ReleaseParser(psr);

	g_pSPI = (StoredPacketInfo*)malloc(sizeof(StoredPacketInfo) * maxSession_);
	for (ULONGLONG i = 0; i < maxSession_; ++i)
		new(g_pSPI + i)StoredPacketInfo{ overSend_ };

	for (DWORD i = 0; i < IOCP_WORKER_THREAD_NUM_; ++i)
		ResumeThread(hIOCPWorkerThreadArr_[i]);

	return true;
};

void IOCP_ECHO_DUMMY::OnRecv(ULONGLONG id, Packet* pPacket)
{
	MEMORY_LOG(ON_RECV, id);
	StoredPacketInfo* pCurSPI = g_pSPI + Session::GET_SESSION_INDEX(id);

	ULONGLONG payLoad;
	(*pPacket) >> payLoad;
	if (payLoad == LOGIN_PAYLOAD)
	{
		OverSend_COMMON(id, pCurSPI, pCurSPI->LastSendNum_);
		return;
	}

	payLoad ^= id;
	ULONGLONG curStoredPayload = pCurSPI->GetCurrentStoredPayLoad();
	if (curStoredPayload != payLoad)
		__debugbreak();


	if (pCurSPI->IsAllRecved() == false)
		return;

	if (rand() % 100 + 1 < RAND_DISCONNECT)
	{
		Disconnect(id);
		return;
	}

	OverSend_COMMON(id, pCurSPI, payLoad);
}

// 구현 안함
void IOCP_ECHO_DUMMY::OnError(ULONGLONG id, int errorType, Packet* pRcvdPacket)
{
	return;
}

void IOCP_ECHO_DUMMY::OnConnect(ULONGLONG id)
{
	StoredPacketInfo* pCurSPI = g_pSPI + Session::GET_SESSION_INDEX(id);
	pCurSPI->Init();
}

void IOCP_ECHO_DUMMY::OnRelease(ULONGLONG id)
{
	return;
}

void IOCP_ECHO_DUMMY::OnConnectFailed(ULONGLONG id)
{
	return;
}

IOCP_ECHO_DUMMY::~IOCP_ECHO_DUMMY()
{
	for (ULONGLONG i = 0; i < maxSession_; ++i)
		delete[] g_pSPI[i].ppPacketPtrArr_;

	free(g_pSPI);
	return;
}

void IOCP_ECHO_DUMMY::SendPacket_OVERSEND(ULONGLONG id, Packet** ppPacketArr, ULONGLONG overSend)
{
	Session* pSession = pSessionArr_ + Session::GET_SESSION_INDEX(id);
	long IoCnt = InterlockedIncrement(&pSession->IoCnt_);

	// 이미 RELEASE 진행중이거나 RELEASE된 경우
	if ((IoCnt & Session::RELEASE_FLAG) == Session::RELEASE_FLAG)
	{
		if (InterlockedDecrement(&pSession->IoCnt_) == 0)
			ReleaseSession(pSession);
		return;
	}

	// RELEASE 완료후 다시 세션에 대한 초기화가 완료된경우 즉 재활용
	if (id != pSession->id_)
	{
		if (InterlockedDecrement(&pSession->IoCnt_) == 0)
			ReleaseSession(pSession);
		return;
	}

	// OverSend 갯수만큼 락프리큐에 인큐
	for (ULONGLONG i = 0; i < overSend; ++i)
	{
		Packet* pPacket = ppPacketArr[i];
		pPacket->SetHeader<Lan>();
		pPacket->IncreaseRefCnt();
		pSession->sendPacketQ_.Enqueue(pPacket);
	}

	SendPost(pSession);
	if (InterlockedDecrement(&pSession->IoCnt_) == 0)
		ReleaseSession(pSession);
}

void IOCP_ECHO_DUMMY::OverSend_COMMON(ULONGLONG id, StoredPacketInfo* pCurSPI, ULONGLONG lastSendNum)
{
	Packet* pPacketArr[500];
	for (int i = 0; i < overSend_; ++i)
	{
		Packet* pPacket = PACKET_ALLOC(Lan);
		pPacket->IncreaseRefCnt();
		pPacketArr[i] = pPacket;

		++lastSendNum;
		pCurSPI->Write(lastSendNum);
		ULONGLONG payload = id ^ lastSendNum;
		*pPacket << payload;
	}

	SendPacket_OVERSEND(id, pPacketArr, overSend_);

	for (int i = 0; i < overSend_; ++i)
	{
		if (pPacketArr[i]->DecrementRefCnt() == 0)
			PACKET_FREE(pPacketArr[i]);
	}
	// 마지막으로 보낸 숫자 저장
	pCurSPI->LastSendNum_ = lastSendNum;
}
