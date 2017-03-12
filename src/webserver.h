/* 
    @author Ayoub Serti
    @email  ayb.serti@gmail.com
 */

#pragma once
#include <nan.h>
#include <nan_object_wrap.h>

#include <uv.h>

class WebServer
{


};


class WebServerWrap : public Nan::ObjectWrap
{
    
};


template<typename T>
class ListNodeMiror {

    ListNodeMiror*  prev_;
    ListNodeMiror*  next_;
};

struct CallbackMirror {
    void*  fn;
    void*  dn;

};


/*
  inheritence schema
  ConnectionWrap : StreamWrap : (HandleWrap,           StreamBase)
                                   : AsyncWrap              :StreamResource 
                                      :BaseObject
*/
struct ConnectionTemplateWrapMirror 
{ 
	//vtable
  void* vfunc1;
  void* vfunc2;
  //BaseObject
  v8::Persistent<v8::Object> persistent_handle_;
  node::Environment* env_;

  //AsyncWrap
  uint32_t bits_;
  const int64_t uid_;

  //HandleWrap
   ListNodeMiror<void>  handle_rap_queue;
   uv_handle_t* const handle__;


   //StreamResource
   CallbackMirror cb1;
   CallbackMirror cb2;
   CallbackMirror cb3;
   uint64_t bytes_read_;

   //StreamBase:
   node::Environment* env__;
   bool consumed_;

   //StreamWrap
   uv_stream_t* const stream_;

   //ConnectionWrap
   uv_tcp_t handle_;

};