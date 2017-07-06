#include "webserver.h"

#include "node_header.h"

#include "compression.h"

#include <iostream>
#include <thread>
#include <functional>

#include <string>

template<typename T>
struct protect_wrapper : T
{
	protect_wrapper(const T& t) : T(t)
	{

	}

	protect_wrapper(T&& t) : T(std::move(t))
	{

	}
};

template<typename T>
typename std::enable_if< !std::is_bind_expression< typename std::decay<T>::type >::value,
	T&& >::type
	protect(T&& t)
{
	return std::forward<T>(t);
}

template<typename T>
typename std::enable_if< std::is_bind_expression< typename std::decay<T>::type >::value,
	protect_wrapper<typename std::decay<T>::type > >::type
	protect(T&& t)
{
	return protect_wrapper<typename std::decay<T>::type >(std::forward<T>(t));
}


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


NAN_METHOD(Forward)
{
	
	if (info.Length() > 0)
	{
		v8::Local<v8::Object> obj = Nan::To<v8::Object>(info[0]).ToLocalChecked();
		
		v8::Local<v8::String> data = Nan::To< v8::String>(info[1]).ToLocalChecked();

		v8::String::Utf8Value utfData(data);
		
		std::shared_ptr<std::string> strData(new std::string(*utfData, utfData.length()));

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
			response->set_header("Content-Encoding", "deflate");
			response->set_status(HTTP_STATUS_OK);
			response->set_header("Connection", "Close");
			response->append_body(strData->c_str(), strData->size());


			 auto dummy_compressor = [](const char* raw_buf, size_t raw_buf_size, char* outBuf, size_t& outLen)->size_t {
				//compressor dummy
				//memcpy(outBuf, raw_buf, raw_buf_size);
				//outLen = raw_buf_size;
				//return 0;

				// zlib struct
				z_stream defstream;
				defstream.zalloc = Z_NULL;
				defstream.zfree = Z_NULL;
				defstream.opaque = Z_NULL;
				defstream.avail_in = raw_buf_size;
				defstream.next_in = (Bytef *)raw_buf;
				defstream.avail_out = outLen;
				defstream.next_out = (Bytef *)outBuf;

				deflateInit(&defstream, Z_BEST_COMPRESSION);
				if (deflate(&defstream, Z_FINISH) == Z_STREAM_END)
				{
					deflateEnd(&defstream);
					outLen = defstream.total_out;
					return 0;

				}
				else
				{
					//error or need more out buf
					outLen = deflateBound(&defstream, raw_buf_size);
					deflateEnd(&defstream);
					return 0;
				}

			};

			 response->set_compressor(dummy_compressor);
			
			auto Sender = [](TcpSocket* socket, const char* buf, size_t len, function<void(const asio::error_code&, std::size_t)>& WriteHandler) {
				socket->async_send(asio::buffer(buf, len), WriteHandler);
			};


			auto HandlerWriter = [](std::shared_ptr<HTTPResponse> response , TcpSocket* socket, const asio::error_code& err, std::size_t len) {

				//check if every thing was sent
				response->dec_sending_size(len);
				if (response->get_sending_size() == 0)
				{
					asio::error_code sh_ec;
					socket->shutdown(asio::socket_base::shutdown_both, sh_ec);

					delete socket;

					//clear body and compressed body
					response->clear_body();
					response->clear_compressed_body();

				}

			};

			function<void(const asio::error_code&,std::size_t len)> simplified_handlewriter = std::bind(HandlerWriter, response, asocket, std::placeholders::_1, std::placeholders::_2);
			auto Send_simplified = std::bind(Sender, asocket, std::placeholders::_1, std::placeholders::_2, std::ref(simplified_handlewriter));
			send_reponse(response,true,  std::move(Send_simplified)	,
				[&](std::shared_ptr<HTTPResponse>,const std::error_code& ec, size_t len) {

				//May cache HTTPResponse, or log ELF 
				if (!ec)
				{
					
				}
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