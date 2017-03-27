#include "webserver.h"

#include "compression.h"

AsyncWriteCompressableStream::AsyncWriteCompressableStream(TcpSocket& socket)
:socket_(socket)
{

}

asio::io_service& AsyncWriteCompressableStream::io_service()
{
	return socket_.get_io_service();
}

template<typename CB, typename HW>
void AsyncWriteCompressableStream::async_write_some(CB buffer, HW callback) 
{
	socket_.async_write(buffer, callback);
}