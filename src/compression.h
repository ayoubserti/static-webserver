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

   char out_buf_[MAX_CHUNK_SIZE*2];

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
		   //one single chunck
		   std::copy(buffer.begin(), buffer.end(), flate_buf_);
		   std::size_t len = MAX_CHUNK_SIZE*2;
		   do_compress(flate_buf_, chunck_size, out_buf_, len);
		   if (len > MAX_CHUNK_SIZE*2)
		   {
			   // compressed buf > flate buf 
			   // compression unsuccessful
			   std::cerr << "A fatal error" << std::endl;

		   }
		   else
		   {
			   socket_.async_send(asio::buffer(out_buf, len), [&callback](const asio::error_code& ec, std::size_t write_len) {
				   if (!ec)
				   {
					   callback();
				   }
			   });
		   }
	   }
	   else
	   {
		   //multiple chuncks
		   std::copy(buffer.begin(), buffer.begin() + MAX_CHUNK_SIZE, flate_buf_);
		   std::size_t len = MAX_CHUNK_SIZE*2;
		   do_compress(flate_buf_, MAX_CHUNK_SIZE, out_buf_, len);
		   //TODO: check error while compression flate_buf_ 
		   socket_.async_send(asio::buffer(out_buf, len), [&buffer](const asio::error_code& ec, std::size_t write_len) {
			   size_t waiting_len = std::distance(buffer.begin() + MAX_CHUNK_SIZE, buffer.end());
			   asio::async_write(*(AsyncWriteCompressableStream*)this, asio::buffer(buffer.begin() + MAX_CHUNK_SIZE, buffer.end()),
				   [&buffer,&callback,waiting_len](const asio::error_code& ec2, std::size_t write_len) {
				   //multiple time callback invoke
				   if(!ec2)
					   if (write_len == waiting_len)
					   {
						 // about the last chunck
						   callback();
				       }
			   });
		   });



	   }
	   socket_.async_send(buffer, callback);
   }

};



#endif //__COMPRESSION_H__