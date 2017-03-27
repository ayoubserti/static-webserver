#ifndef __COMPRESSION_H__
#define __COMPRESSION_H__

class AsyncWriteCompressableStream 
{
   AsyncWriteCompressableStream( AsyncWriteCompressableStream& )  = delete;
   TcpSocket& socket_;

   public:

   
   AsyncWriteCompressableStream(TcpSocket& socket);

   io_service& io_service();
   template<typename CB, typename HW>
   void async_write_some(CB buffer, HW callback);

};



#endif //__COMPRESSION_H__