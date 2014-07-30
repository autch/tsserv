
#include "tsserv.h"

void http_connect_cb(struct evhttp_request* req, void* user)
{
	struct tssparent_context* ctx = user;
	tsclient* client = tsclientNew();

	client->req = req;
	ctx->clients = tsclientPrepend(ctx->clients, client);

	evhttp_add_header(req->output_headers, "Content-Type", "video/mpegts");

	evhttp_send_reply_start(req, HTTP_OK, "OK");
}
