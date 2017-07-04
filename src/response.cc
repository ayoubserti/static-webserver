#include "response.h"

void compress_body(std::shared_ptr<HTTPResponse> res, char*& outBuf, size_t& len, const function<size_t(const char*, size_t, char*, size_t&)>&& Compressor, const function<void(std::shared_ptr<HTTPResponse>, const char*, size_t)>&& Compelation)
{
	if (res->body_ != nullptr)
	{
		//not optimized
		size_t chunklen = res->body_length;
		size_t  total_len = 0;

		char* buffer = reinterpret_cast<char*>(::malloc(res->body_length));
		outBuf = buffer;
		len = res->body_length;
		size_t remineder = Compressor(res->body_, res->body_length, buffer, chunklen);
		total_len += chunklen;
		while (remineder != 0)
		{
			chunklen = remineder;

			outBuf = reinterpret_cast<char*>(::realloc(outBuf, total_len));
			remineder = Compressor(res->body_ + res->body_length - remineder, remineder, outBuf + total_len, chunklen);
			total_len += chunklen;
		}

		res->set_header("Content-Length", total_len);
		res->compressed_body_ = outBuf;
		res->compressed_body_length_ = total_len;
		Compelation(res, outBuf, total_len);


	}
}

void send_reponse(std::shared_ptr<HTTPResponse> res, bool sendHeader,  function<void(const char*, size_t)>&& Sender, const function<void(std::shared_ptr<HTTPResponse>response, const std::error_code&, size_t len)>&& Compelation)
{
	
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

			
			if (sendHeader)
			{
				response->stringify();
				
				Sender(response->stringified_headers_, response->stringified_headers_len_);
				
				res->inc_sending_size(response->stringified_headers_len_);

				//mem leaked
			}
			Sender(buffer, total_len);
			res->inc_sending_size(total_len);
			std::error_code ec;
			Compelation(response, ec, total_len);
		});

	}
}
