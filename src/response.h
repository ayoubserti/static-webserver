/*
    @author Ayoub Serti
    @email  ayb.serti@gmail.com
    
*/
#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <list>
#include "http_parser.h"


using std::string;
using std::unordered_map;
using std::function;
using std::list;


struct http_status_stingifier
{
	int num;
	string msg;
};

static http_status_stingifier all_http_status[] = {
#define XX(num,name,msg) {num , #msg},
	HTTP_STATUS_MAP(XX)
#undef XX

};

/*
    @abstract  Reponse represent an Http response; basically this class is helps serializing Http Response
*/
class HTTPResponse : std::enable_shared_from_this<HTTPResponse>
{
    
public:
	typedef unordered_map<string, string> HeadersMap;
	enum class eCompressionMethod {
		eCompressionNone = 0,
		eCompressionDeflate,
		eCompressionGzip,

		eCompressionUnknown = -1
	};
private:

	http_status status_;
	string		status_str_;
	HeadersMap	headers_;
	char*		body_ =nullptr;
	std::size_t body_length;
	eCompressionMethod compression_method_ = eCompressionMethod::eCompressionNone;
	friend
	void compress_body(std::shared_ptr<HTTPResponse> res, char*& outBuf, size_t& len, const function<size_t(const char*, size_t, char*, size_t&)>&& Compressor, const function<void(std::shared_ptr<HTTPResponse>, const char*, size_t)>&& Compelation);

	//Send packet
	friend
	void send(std::shared_ptr<HTTPResponse> res, bool sendHeader, const function<void(const char*, size_t)>&& Sender, const function<void(std::shared_ptr<HTTPResponse>response, const std::error_code&, size_t len)>&& Compelation);


public:

	string stringify()
	{
		string result{"HTTP/1.1 "};
		result += std::to_string(status_);
		result += " " + status_str_ + "\n";
		for (auto& i : headers_)
		{
			result += i.first + ": " + i.second + "\n";
		}
		result += "\n";
		return result;
	}

	void set_status(http_status status)
	{
		status_ = status;
		for (auto& i : all_http_status)
		{
			if (i.num == status_)
			{
				status_str_ = i.msg;
				break;
			}
				
		}
	}

	void set_header(const string& name, std::size_t value)
	{

		headers_[name] = std::to_string((long long)value);
	}


	void set_header(const string& name, const string& value)
	{
		headers_[name] = value;
	}
	void append_body(const char* buf, size_t len)
	{
		body_ = reinterpret_cast<char*>(::malloc(len));
		memcpy(body_, buf, len);
		body_length = len;
	}

	~HTTPResponse(){
		if (body_ != nullptr)
			::free(body_);
	}
	

	size_t get_body_length() { return body_length; }

	const char* get_body() { return body_; }

	
	
};
void compress_body(std::shared_ptr<HTTPResponse> res, char*& outBuf, size_t& len, const function<size_t(const char*, size_t, char*, size_t&)>&& Compressor, const function<void(std::shared_ptr<HTTPResponse>, const char*, size_t)>&& Compelation);

void send(std::shared_ptr<HTTPResponse> res, bool sendHeader, const function<void(const char*, size_t)>&& Sender, const function<void(std::shared_ptr<HTTPResponse>response, const std::error_code&, size_t len)>&& Compelation);