#include "webserver.h"

#include "node_header.h"

#include "compression.h"

#include <iostream>
#include <thread>
#include <functional>

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

static void Sender(TcpSocket* socket, const char* buf, size_t len, function<void(const asio::error_code&, std::size_t)> WriteHandler) {
	socket->async_send(asio::buffer(buf, len), WriteHandler);
};


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
		
		
		
		
		webserver.GetIoService().dispatch([asocket, strData]() {
			//executed in static-server thread

			std::shared_ptr<HTTPResponse> response(new HTTPResponse());
			response->set_header("Content-Type", "text/plain");
			response->set_status(HTTP_STATUS_OK);
			response->set_header("Connection", "Keep-alive");
			//response->set_header("Date", "Tue, 27 Jun 2017 01:27:37 GMT");
			//response->set_header("Content-Encoding", "deflate");
			response->append_body(strData->c_str(), strData->size());

			auto Completion = [](std::error_code& err, size_t len) -> void {


			};

			
			
			//std::function<void(const char* buf, size_t len)> Sender
			response->send(response,true, strData->c_str(), strData->size(), [&](const char* buf, size_t len) {
				Sender(asocket, buf, len, [](const asio::error_code& err, std::size_t len) {

					if (!err)
					{
						std::cout << "size sent " << len << std::endl;
					}
					else
					{
						std::cout << err.message() << std::endl;
					}
				});
			}, [](std::shared_ptr<HTTPResponse>,const std::error_code& ec, size_t len) {

				std::cout << "Finished" << std::endl;
			});

			//size_t compsize = strData->size() * (1.5);
			

			//response->set_header("Content-Lenght", compsize);

			//send headers sync
			//asocket->send(asio::buffer(response.stringify()));
			


			
/*			asio::async_write(*stream,asio::buffer(compressed, compsize),
				[stream,asocket,compressed](const asio::error_code& ec, std::size_t len) {
				
				//shutdown socket after write
				asio::error_code sh_ec;
				asocket->shutdown(asio::socket_base::shutdown_both,sh_ec);
				delete asocket;
				delete compressed;
				delete stream;
			});
			*/
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