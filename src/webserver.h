/* 
    @author Ayoub Serti
    @email  ayb.serti@gmail.com
 */

#pragma once
#include <nan.h>
#include <nan_object_wrap.h>

#include <uv.h>
#include "response.h"

#ifdef _WIN32
#define  _WIN32_WINNT  0x0601
#define  ASIO_DISABLE_IOCP 1
#endif
#define  ASIO_STANDALONE 1
#include "asio.hpp"

using TcpSocket = asio::ip::tcp::socket;
using asio::io_service;

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

class WebServer
{
	bool started_ = false;
	io_service io_service_;

	static WebServer* sInstance;

	WebServer(const WebServer&) = delete;
	WebServer(WebServer&&) = delete;

	WebServer();

	static void thread_work(WebServer&);

	void set_thread_name( std::thread& worker);
public:

	static WebServer& Get();
	
	io_service& GetIoService(); 

	void start();

};






class WebServerWrap : public Nan::ObjectWrap
{
    
};

