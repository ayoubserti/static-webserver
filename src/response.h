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


typedef  function<size_t(const char*, size_t, char*, size_t&)> CompressorFunction;

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
	std::size_t body_length =0;
	char*		compressed_body_ = nullptr;
	size_t		compressed_body_length_ = 0;


	CompressorFunction compressor_;

    
	eCompressionMethod compression_method_ = eCompressionMethod::eCompressionNone;
	friend
	void compress_body(std::shared_ptr<HTTPResponse> res, char*& outBuf, size_t& len, const CompressorFunction& Compressor, const function<void(std::shared_ptr<HTTPResponse>, const char*, size_t)>&& Compelation);

	//Send packet
	friend
	void send_reponse(std::shared_ptr<HTTPResponse> res, bool sendHeader,  function<void(const char*, size_t)>&& Sender, const function<void(std::shared_ptr<HTTPResponse>response, const std::error_code&, size_t len)>&& Compelation);


	size_t sending_size_ = 0;

public:

	char*		stringified_headers_ = nullptr;
	size_t		stringified_headers_len_ = 0;

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
		if (stringified_headers_) ::free(stringified_headers_);
		stringified_headers_len_ = result.size();
		stringified_headers_ = (char*)::malloc(stringified_headers_len_);
		::memcpy(stringified_headers_, result.c_str(), stringified_headers_len_);
		
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

	void clear_body()
	{
		if (body_ != nullptr)
			std::free(body_);
		body_ = nullptr;
		body_length = 0;

	}

	void clear_compressed_body()
	{
		if (compressed_body_ != nullptr)
			std::free(compressed_body_);
		compressed_body_ = nullptr;
		compressed_body_length_ = 0;
	}

	~HTTPResponse(){
		if (body_ != nullptr)
			::free(body_);
		if (compressed_body_ != nullptr)
			::free(compressed_body_);
		
		::free(stringified_headers_);
	}
	

	size_t get_body_length() { return body_length; }

	const char* get_body() { return body_; }

	void inc_sending_size(size_t s) { sending_size_ += s; }

	void dec_sending_size(size_t s) { sending_size_ -= s; }

	size_t get_sending_size() { return sending_size_; }

	void   set_compressor(CompressorFunction&& func) { compressor_ = func; }
	
	
};
void compress_body(std::shared_ptr<HTTPResponse> res, char*& outBuf, size_t& len, const CompressorFunction& Compressor, const function<void(std::shared_ptr<HTTPResponse>, const char*, size_t)>&& Compelation);

void send_reponse(std::shared_ptr<HTTPResponse> res, bool sendHeader,  function<void(const char*, size_t)>&& Sender, const function<void(std::shared_ptr<HTTPResponse>response, const std::error_code&, size_t len)>&& Compelation);