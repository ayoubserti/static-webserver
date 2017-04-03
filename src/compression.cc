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



void AsyncWriteCompressableStream::do_compress(const char* in, size_t len, char* out, size_t& out_len){

	
	// zlib struct
	z_stream defstream;
	defstream.zalloc = Z_NULL;
	defstream.zfree = Z_NULL;
	defstream.opaque = Z_NULL;
	defstream.avail_in = len; 
	defstream.next_in = (Bytef *)in; 
	defstream.avail_out = out_len; 
	defstream.next_out = (Bytef *)out; 

	deflateInit(&defstream, Z_BEST_COMPRESSION);
	if (deflate(&defstream, Z_FINISH) == Z_STREAM_END)
	{
		deflateEnd(&defstream);
		out_len = defstream.total_out;

	}
	else
	{
		//error or need more out buf
		out_len = deflateBound(&defstream, len);
		deflateEnd(&defstream);
	}

}