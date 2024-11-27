#pragma once
struct StoredPacketInfo
{
	static inline ULONGLONG overSend;
	ULONGLONG LastSendNum_;
	ULONGLONG* ppPacketPtrArr_;
	DWORD recvIdx_;
	DWORD sendIdx_;
	StoredPacketInfo(ULONGLONG overSend)
	{
		Init();
		ppPacketPtrArr_ = new ULONGLONG[overSend];
	}

	void Init()
	{
		// 오버플로우를 유도
		LastSendNum_ = UINT64_MAX;
		sendIdx_ = UINT32_MAX;
		recvIdx_ = UINT32_MAX;
	}

	void Write(ULONGLONG lastSendNum)
	{
		sendIdx_ = ((++sendIdx_) % overSend);
		ppPacketPtrArr_[sendIdx_] = lastSendNum;
	}

	ULONGLONG GetCurrentStoredPayLoad()
	{
		recvIdx_ = ((++recvIdx_) % overSend);
		return ppPacketPtrArr_[recvIdx_];
	}

	__forceinline bool IsAllRecved()
	{
		return (recvIdx_ == sendIdx_) && (recvIdx_ == overSend - 1);
	}
};
