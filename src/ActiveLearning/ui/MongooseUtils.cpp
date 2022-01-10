#include "MongooseUtils.h"

#include <string>
#include <sstream>
#include "mongoose/mongoose.h"

using std::string;
using std::stringstream;

const char * ajax_reply_start =
  "HTTP/1.1 200 OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: application/x-javascript\r\n"
  "\r\n";

string MongooseUtils::get_qsvar(const struct mg_request_info *request_info,
										 const char* name, int max_len) 
{
	char* buf = new char[max_len];
	const char* qs = request_info->query_string;
	mg_get_var(qs, strlen(qs == NULL ? "" : qs), name, buf, max_len);
	string ret(buf);
	delete buf;
	return ret;
}

void MongooseUtils::send_json(struct mg_connection* conn,
	const struct mg_request_info *request_info, const std::string& json) 
{
	// if this is a JSONP call, we need to wrap our return in the
	// callback function.
	char callback_name[64];
	const char* query_string = request_info->query_string;
	mg_get_var(query_string, (query_string != NULL) ? strlen(query_string) : 0,
		"callback", callback_name, sizeof(callback_name));
	bool needs_callback = callback_name[0] != '\0';

	stringstream msg;
	if (needs_callback) {
		msg << callback_name << "(";
	}
	msg << json;
	if (needs_callback) {
		msg << ")";
	}
	http_write(conn, msg.str());
}

void MongooseUtils::http_write(struct mg_connection* conn, const string& str)
{
	const char* c_str = str.c_str();
	int len = strlen(c_str);
	mg_write(conn, c_str, len);
}

void MongooseUtils::begin_ajax_reply(struct mg_connection* conn) {
	MongooseUtils::http_write(conn, ajax_reply_start);
}
