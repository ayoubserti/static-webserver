#include "response.h"

void HTTPResponse::compress_body(std::shared_ptr<HTTPResponse> res, char*& outBuf, size_t& len, const function<size_t(const char*, size_t, char*, size_t&)>&& Compressor, const function<void(std::shared_ptr<HTTPResponse>, const char*, size_t)>&& Compelation)
{
	if (body_ != nullptr)
	{
		//not optimized
		size_t chunklen = body_length;
		size_t  total_len = 0;

		char* buffer = reinterpret_cast<char*>(::malloc(body_length));
		outBuf = buffer;
		len = body_length;
		size_t remineder = Compressor(body_, body_length, buffer, chunklen);
		total_len += chunklen;
		while (remineder != 0)
		{
			chunklen = remineder;

			outBuf = reinterpret_cast<char*>(::realloc(outBuf, total_len));
			remineder = Compressor(body_ + body_length - remineder, remineder, outBuf + total_len, chunklen);
			total_len += chunklen;
		}

		set_header("Content-Length", total_len);
		//std::shared_ptr<HTTPResponse> res(this);
		Compelation(res, outBuf, total_len);


	}
}

void HTTPResponse::send(std::shared_ptr<HTTPResponse> res, bool sendHeader, const char* buf, size_t len, const function<void(const char*, size_t)>&& Sender, const function<void(std::shared_ptr<HTTPResponse>response, const std::error_code&, size_t len)>&& Compelation)
{

	//first send headers

	if (buf != nullptr && len > 0)
	{
		char* buffer = nullptr;
		size_t total_len;
		const auto dummy_compressor = [&](const char* raw_buf, size_t raw_buf_size, char* outBuf, size_t& outLen)->size_t {
			//compressor dummy
			memcpy(outBuf, raw_buf, raw_buf_size);
			outLen = raw_buf_size;
			return 0;

		};
		compress_body(res, buffer, total_len, std::move(dummy_compressor), [&](std::shared_ptr<HTTPResponse> response, const char* buf_to_send, size_t len_buf_to_send) {

			//response->send(false, buffer, total_len, std::move(Sender), std::move(Compelation));
			if (sendHeader)
			{
				string* headersStr = new string(res->stringify());

				Sender(headersStr->c_str(), headersStr->size());
			}
			Sender(buffer, total_len);
			std::error_code ec;
			Compelation(response, ec, total_len);
		});

	}
}
