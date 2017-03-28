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

