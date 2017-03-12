#include "webserver.h"

#include "node_header.h"

#include <iostream>
#include <thread>
#include <functional>
#define  _WIN32_WINNT  0x0601
#define  ASIO_DISABLE_IOCP 1
#define  ASIO_STANDALONE 1
#include "asio.hpp"


#include <zconf.h>
#include <zlib.h>

#include <string>



char* compress(const char* in, size_t len,size_t& out) {


	char* res = new char[out];
	// zlib struct
	z_stream defstream;
	defstream.zalloc = Z_NULL;
	defstream.zfree = Z_NULL;
	defstream.opaque = Z_NULL;
	// setup "a" as the input and "b" as the compressed output
	defstream.avail_in = len; // size of input, string + terminator
	defstream.next_in = (Bytef *)in; // input char array
	defstream.avail_out = out; // size of output
	defstream.next_out = (Bytef *)res; // output char array

	
	//deflateInit2(&defstream, Z_BEST_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 3, Z_HUFFMAN_ONLY);

	deflateInit(&defstream, Z_BEST_COMPRESSION);
	deflate(&defstream, Z_FINISH);
	deflateEnd(&defstream);
	out = defstream.total_out;
	
	return res;
}

#ifdef _WIN32
#include "Windows.h"
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

#endif

using TcpSocket = asio::ip::tcp::socket;

asio::io_service* gIOService;

void launcher(asio::io_service& io_service) {

	
	asio::io_service::work dummy(io_service);

	io_service.run();
};





NAN_METHOD(Forward)
{
	if (info.Length() > 0)
	{
		v8::Local<v8::Object> obj = Nan::To<v8::Object>(info[0]).ToLocalChecked();
		
		node::TCPWrap * mirror = reinterpret_cast<node::TCPWrap *>(Nan::GetInternalFieldPointer(obj, 0));

		uv_tcp_t* handle = mirror->UVHandle();
		
		TcpSocket::native_handle_type sock;
#if WIN32
		sock = handle->socket;
		WSAPROTOCOL_INFO pi;
		WSADuplicateSocket(sock, GetCurrentProcessId(), &pi);
		SOCKET socketDup = WSASocket(pi.iAddressFamily/*AF_INET*/, pi.iSocketType/*SOCK_STREAM*/,
			pi.iProtocol/*IPPROTO_TCP*/, &pi, 0, 0);
		TcpSocket* asocket = new TcpSocket(*gIOService);
		asio::error_code ec;
		asocket->assign(asio::ip::tcp::v6(), socketDup,ec);
		gIOService->dispatch([asocket]() {
			//executed in static-server thread
			std::string* strRes =new std::string( "HTTP/1.1 200 OK\nConnection: Closed\nContent-Type:text/plain\nContent-Encoding:deflate\nContent-Length:"); 
			//"26\n\n Hello From static - server\n";
			char * body = " Hello From static - server\n";
			size_t compsize = 50;
			char * compressed = compress(body, strlen(body), compsize);
			*strRes += std::to_string((long long)compsize);
			*strRes += "\n\n";
			std::string* strbody = new std::string(compressed, compsize);
			strRes->append(strbody->begin(), strbody->end());

			asocket->async_send(asio::buffer(strRes->c_str(),strRes->size()),
				[asocket,compressed](const asio::error_code& ec, std::size_t len) {
				std::cout << "From Asio Thread" << std::endl;
				//shutdown socket after write
				asocket->shutdown(asio::socket_base::shutdown_both);
				delete asocket;
			});

		});

		
		


#endif 

			
		
	}
}

NAN_MODULE_INIT(Init) {
	gIOService = new asio::io_service();

	std::thread asioWorker(launcher, std::ref(*gIOService));
	
#if WIN32 && _DEBUG
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = "static-server";
	info.dwThreadID = ::GetThreadId(static_cast<HANDLE>(asioWorker.native_handle()));
	info.dwFlags = 0;
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
	
	try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	catch (...)
	{
	}
#pragma warning(pop)  
#endif
	asioWorker.detach();
	
 Nan::Set(target,Nan::New<v8::String>("forward").ToLocalChecked(),Nan::GetFunction(
      Nan::New<v8::FunctionTemplate>(Forward)).ToLocalChecked());
}

NODE_MODULE(webserver, Init)