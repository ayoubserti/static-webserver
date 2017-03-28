#ifndef __COMPRESSION_H__
#define __COMPRESSION_H__


#include <zconf.h>
#include <zlib.h>

#define MAX_CHUNK_SIZE 1024

class AsyncWriteCompressableStream 
{
   AsyncWriteCompressableStream( AsyncWriteCompressableStream& )  = delete;
   TcpSocket& socket_;

   void do_compress(const char* in, size_t len, char* out, size_t& out_len);

   char  flate_buf_[MAX_CHUNK_SIZE];

   char out_buf_[MAX_CHUNK_SIZE];

   public:

   
    AsyncWriteCompressableStream(TcpSocket& socket);

   io_service& io_service();
   template<typename CB, typename HW>
   void async_write_some(const CB& buffer, HW callback)
   {
	   // todo: add compile time check, if really a ConstBufferSequence
	   typedef const_iterator typename CB::const_iterator;
	   
	   //chuncking data by MAX_CHUNK_SIZE

	   size_t chunck_size = std::distance(buffer.begin(), buffer.end());
	   
	   if (chunck_size <= MAX_CHUNK_SIZE)
	   {
		   std::copy(buffer.begin(), buffer.end(), flate_buf_);
		   std::size_t len = chunck_size;
		   do_compress(flate_buf_, chunck_size, out_buf_, len);
		   if (len > MAX_CHUNK_SIZE)
		   {
			   // compressed buf > flate buf

		   }
	   }
	   socket_.async_send(buffer, callback);
   }

};



#endif //__COMPRESSION_H__