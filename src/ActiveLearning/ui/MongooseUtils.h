#ifndef _MONGOOSE_UTILS_H_
#define _MONGOOSE_UTILS_H_

#include <string>

struct mg_connection;
struct mg_request_info;

namespace MongooseUtils {
	// send a response to an AJAX query
	void send_json(struct mg_connection* conn,
		const struct mg_request_info *request_info, const std::string& json);
	// convenience wrapper for writing a std::string across an HTTP connection
	void http_write(struct mg_connection* conn, const std::string& str);

	void begin_ajax_reply(struct mg_connection* conn);

	// extract a variable from the query string sent by the client
	std::string get_qsvar(const struct mg_request_info *request_info,
										 const char* name, int max_len); 
};
#endif
