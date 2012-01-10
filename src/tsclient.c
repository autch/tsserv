
#include "tsserv.h"
#include <stdlib.h>

#define NEXT(p)  ((p)->next)

void tsclientDestroy(tsclient* p)
{
	if(p->bev != NULL)
		bufferevent_free(p->bev);
}

tsclient* tsclientNew()
{
	return calloc(1, sizeof(tsclient));
}

tsclient* tsclientFindTail(tsclient* head)
{
	tsclient* p;

	for(p = head; p && NEXT(p); p = NEXT(p))
		;
	return p;
}

tsclient* tsclientFindByBEV(tsclient* head, struct bufferevent* bev)
{
	tsclient* p;

	for(p = head; p; p = NEXT(p))
		if(p->bev == bev) return p;

	return NULL;
}


tsclient* tsclientPick(tsclient* head, int (*proc)(tsclient*, void*), void* user)
{
	tsclient* p;

	for(p = head; p; p = NEXT(p))
		if(proc(p, user)) return p;

	return NULL;
}

tsclient* tsclientPrepend(tsclient* head, tsclient* node)
{
	if(NEXT(node) == NULL)
		NEXT(node) = head;
	else
	{
		tsclient* tail;
		tail = tsclientFindTail(node);
		NEXT(tail) = head;
	}
	return node;
}

void tsclientFree(tsclient* top)
{
	tsclient* p = top;
	tsclient* q;

	while(p)
	{
		q = NEXT(p);
		tsclientDestroy(p);
		free(p);
		p = q;
	}
}


tsclient* tsclientDelete(tsclient* top, tsclient* to_remove)
{
	tsclient* p = top;
	tsclient* q;

	if(top == to_remove)
	{
		q = NEXT(top);
		NEXT(top) = NULL;
		tsclientFree(top);
		return q;
	}

	while(p)
	{
		q = NEXT(p);

		if(q == to_remove)
		{
			NEXT(p) = NEXT(q);
			NEXT(q) = NULL;
			tsclientFree(q);
			p = NEXT(p);
		}
		else
			p = q;
	}

	return top;
}

tsclient* tsclientRemove(tsclient* top, tsclient* to_remove)
{
	tsclient* p = top;
	tsclient* q;

	if(top == to_remove)
	{
		q = NEXT(top);
		NEXT(top) = NULL;
		return q;
	}

	while(p)
	{
		q = NEXT(p);

		if(q == to_remove)
		{
			NEXT(p) = NEXT(q);
			NEXT(q) = NULL;
			p = NEXT(p);
		}
		else
			p = q;
	}

	return top;
}
