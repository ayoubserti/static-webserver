#include "webserver.h"

#include "node_header.h"

#include "compression.h"

#include <iostream>
#include <thread>
#include <functional>


#include <zconf.h>
#include <zlib.h>

#include <string>



WebServer::WebServer()
{
	
}

WebServer* WebServer::sInstance = nullptr;

WebServer& WebServer::Get()
{
	if (sInstance == nullptr)
	{
		sInstance = new WebServer();
	}
	return *sInstance;
}

asio::io_service& WebServer::GetIoService()
{
	return io_service_;
}

void WebServer::thread_work(WebServer& webServer)
{

	asio::io_service::work dummy(webServer.io_service_); //for keeping thread alive

	webServer.io_service_.run();
}

void WebServer::start()
{
	if (!started_)
	{
		std::thread worker(&WebServer::thread_work,std::ref(*this));

		set_thread_name(worker);
		worker.detach();
		started_ = true;
	}
	
}

void WebServer::set_thread_name( std::thread& worker)
{
#if WIN32 && _DEBUG
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = "static-server";
	info.dwThreadID = ::GetThreadId(static_cast<HANDLE>(worker.native_handle()));
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
}

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

	deflateInit(&defstream, Z_BEST_COMPRESSION);
	deflate(&defstream, Z_FINISH);
	deflateEnd(&defstream);
	out = defstream.total_out;
	
	return res;
}


NAN_METHOD(Forward)
{
	
	if (info.Length() > 0)
	{
		v8::Local<v8::Object> obj = Nan::To<v8::Object>(info[0]).ToLocalChecked();
		
		v8::Local<v8::String> data = Nan::To< v8::String>(info[1]).ToLocalChecked();

		v8::String::Utf8Value utfData(data);
		//i know `std::string*` is a bad idea ;(
		std::string* strData = new std::string(*utfData, utfData.length());

		node::TCPWrap * mirror = reinterpret_cast<node::TCPWrap *>(Nan::GetInternalFieldPointer(obj, 0));

		uv_tcp_t* handle = mirror->UVHandle();
		
		TcpSocket::native_handle_type sock;
		
#if WIN32
		TcpSocket::native_handle_type sock2;
		sock2 = handle->socket;
		WSAPROTOCOL_INFO pi;
		WSADuplicateSocket(sock2, GetCurrentProcessId(), &pi);
		sock = WSASocket(pi.iAddressFamily/*AF_INET*/, pi.iSocketType/*SOCK_STREAM*/,
			pi.iProtocol/*IPPROTO_TCP*/, &pi, 0, 0);
#else
		uv_os_fd_t sock1;
		uv_fileno((uv_handle_t*)handle,&sock1);		
		sock = dup(sock1);
#endif
		WebServer& webserver = WebServer::Get();

		

		TcpSocket* asocket = new TcpSocket(webserver.GetIoService());
		
		
		asio::error_code ec;
		asocket->assign(asio::ip::tcp::v4(), sock,ec);
		
		
		AsyncWriteCompressableStream* stream = new AsyncWriteCompressableStream(*asocket);
		
		webserver.GetIoService().dispatch([stream,asocket, strData]() {
			//executed in static-server thread

			HTTPResponse response;
			response.set_header("Content-Type", "text/plain");
			response.set_status(HTTP_STATUS_OK);
			response.set_header("Connection", "Closed");
			response.set_header("Content-Encoding", "deflate");

			size_t compsize = strData->size() * (1.5);
			char * compressed = compress(strData->c_str(), strData->size(), compsize);

			response.set_header("Content-Lenght", compsize);

			//send headers sync
			asocket->send(asio::buffer(response.stringify()));
			
			asio::async_write(*stream,asio::buffer(compressed, compsize),
				[stream,asocket,compressed](const asio::error_code& ec, std::size_t len) {
				
				//shutdown socket after write
				asio::error_code sh_ec;
				asocket->shutdown(asio::socket_base::shutdown_both,sh_ec);
				delete asocket;
				delete compressed;
				delete stream;
			});

		});
	}
}

NAN_MODULE_INIT(Init) {
	
	WebServer& webServer = WebServer::Get();
	webServer.start();

 Nan::Set(target,Nan::New<v8::String>("forward").ToLocalChecked(),Nan::GetFunction(
      Nan::New<v8::FunctionTemplate>(Forward)).ToLocalChecked());
}

NODE_MODULE(webserver, Init)