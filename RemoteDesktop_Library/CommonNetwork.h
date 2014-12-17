#ifndef COMMONNETWORK_H
#define COMMONNETWORK_H
#include "Rect.h"
#include <vector>

#define IVSIZE 16
#define UNAMELEN 256
typedef void(__stdcall *OnConnectCB)();
namespace RemoteDesktop{
	//ensure ths is tighly packed
#pragma pack(push, 1)
	struct Packet_Header{
		int PayloadLen = 0;
		int Packet_Type = -1;
	};	
	struct Packet_Encrypt_Header{
		int PayloadLen = IVSIZE;//should include the IV size 
		char IV[IVSIZE];
	};
	struct KeyEvent_Header{
		int VK;
		char down = 0;
	};		
	struct ConnectionInfo_Header{
		wchar_t UserName[UNAMELEN + 1];
	};
	struct MouseEvent_Header{
		Point pos;
		int HandleID;
		unsigned int Action;
		int wheel;
	};
#pragma pack(pop)
#define NETWORKHEADERSIZE sizeof(Packet_Encrypt_Header)
#define TOTALHEADERSIZE sizeof(Packet_Encrypt_Header) + sizeof(Packet_Header)
#define MAXMESSAGESIZE 1024*1024 *500  //500 MB is the largest single message that is allowed. This is to prevent crashing either the client or server by sending fake packet lengths
#define STARTBUFFERSIZE 1024 *1024 *2
	enum NetworkMessages{
		INVALID,
		RESOLUTIONCHANGE,
		UPDATEREGION,
		MOUSEEVENT,
		KEYEVENT,
		FOLDER,
		FILE,	
		CAD,
		INIT_ENCRYPTION,
		CLIPBOARDCHANGED,
		DISCONNECTANDREMOVE,
		IMAGESETTINGS,
		CONNECTIONINFO
	};
	enum Network_Return{
		FAILED,
		COMPLETED,
		PARTIALLY_COMPLETED
	};
	struct DataPackage{
		DataPackage(const char*d, int l) : data(d), len(l) {}
		const char* data = nullptr;
		int len = 0;

	};
	class NetworkMsg{
	public:
		NetworkMsg(){}
		int payloadlength()const{ auto l = 0; for (auto& a : data) l += a.len; return l; }
		std::vector<DataPackage> data;
		template<class T>void push_back(const T& x){ data.push_back(DataPackage((const char*)&x, sizeof(x))); }
	}; 

	struct Traffic_Stats{
		long long CompressedSendBytes, CompressedRecvBytes;//overall lifetime totals 
		long long UncompressedSendBytes, UncompressedRecvBytes;//overall lifetime totals 

		long long CompressedSendBPS, CompressedRecvBPS;
		long long UncompressedSendBPS, UncompressedRecvBPS;
	};

}


#endif