
#include "tsserv.h"
#include "stdlist_impl.h"
#include <event2/bufferevent.h>

DEFINE_STDLIST_IMPL(struct tssparent_client, tsclient)

void tsclientInit(struct tssparent_client* p)
{
}

void tsclientCopy(struct tssparent_client* from, struct tssparent_client* to)
{
	//
}

void tsclientDestroy(struct tssparent_client* p)
{
	if(p->bev != NULL)
		bufferevent_free(p->bev);
}
