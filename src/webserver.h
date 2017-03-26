/* 
    @author Ayoub Serti
    @email  ayb.serti@gmail.com
 */

#pragma once
#include <nan.h>
#include <nan_object_wrap.h>

#include <uv.h>
#include "response.h"

#define  _WIN32_WINNT  0x0601
#define  ASIO_DISABLE_IOCP 1
#define  ASIO_STANDALONE 1
#include "asio.hpp"

using TcpSocket = asio::ip::tcp::socket;
using asio::io_service;

class WebServer
{
	WebServer(const WebServer&) = delete;
	WebServer(WebServer&&) = delete;

	WebServer();

	static WebServer* sInstance;
	io_service io_service_;

	static void thread_work(WebServer&);

	bool started_ = false;
public:

	static WebServer& Get();
	
	io_service& GetIoService(); 

	void start();

};






class WebServerWrap : public Nan::ObjectWrap
{
    
};

