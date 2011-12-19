/** @file stdlist_impl.h
 *
 * stdlist 線形リストライブラリの実装用マクロ
 *
 * このファイルでは、ユーザ定義の構造体を操作する関数を定義するためのマクロを定義している。
 * マクロの引数にある typename が構造体の型名、prefix は名前重複を避けるための接頭辞に対応する。
 *
 * このライブラリの使い方については stdlist.h に書いているのでそちらを参照。
 * マクロ定義の説明がマクロそのものではなく、それが定義している関数のものであることに注意。
 *
 * @date 2011/01/17
 */

#ifndef STDLIST_IMPL_H_
#define STDLIST_IMPL_H_

#include <stdlib.h>
#include <memory.h>

/**
 * リスト要素を一個、ヒープに確保してポインタを返す。
 *
 * 要素のメンバーはゼロで初期化される。その後そのポインタを引数に
 * ユーザ定義関数の prefix##Init() が呼び出されるので、
 * 特定の値で初期化したり、何らかの初期化処理が必要なときはここで初期化を行う。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_NEW(typename, prefix)	\
typename* prefix##New()								\
{													\
	typename* p = calloc(1, sizeof(typename));		\
	if(p == NULL) return NULL;						\
	prefix##Init(p);								\
	return p;										\
}

/**
 * \p head から始まるリストの末尾要素を検索して返す
 *
 * \p head が NULL だったときは NULL を返す。
 * \p head はリストの本当の先頭要素である必要はない。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_FINDTAIL(typename, prefix)	\
typename* prefix##FindTail(typename* head)				\
{														\
	typename* p;										\
														\
	for(p = head; p && p->_next; p = p->_next)			\
		;												\
	return p;											\
}

/**
 * \p head から始まるリストの要素数を数えて返す
 *
 * \p head が NULL なら 0 を返す。
 * \p head はリストの本当の先頭要素である必要はない。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_COUNT(typename, prefix)	\
size_t prefix##Count(typename* head)				\
{													\
	size_t count = 0;								\
													\
	while(head) {									\
		head = head->_next;							\
		count++;									\
	}												\
	return count;									\
}

/**
 * \p head から始まるリストの各要素を引数に \p proc() を呼び出し、真を返した要素の
 * 個数を数えて返す。
 *
 * \p head が NULL なら 0 を返す。\p user 引数は \p proc() の第二引数として渡される。
 * \p head はリストの本当の先頭要素である必要はない。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_COUNT_IF(typename, prefix)	\
size_t prefix##CountIf(typename* head, int (*proc)(typename*, void*), void* user)	\
{													\
	size_t count = 0;								\
													\
	while(head) {									\
		if(proc(head, user)) count++;				\
		head = head->_next;							\
	}												\
	return count;									\
}

/**
 * \p head から始まるリストの各要素を引数に \p proc() を呼び出し、初めて真を返した要素
 * を返す。
 *
 * \p head が NULL なら 0 を返す。\p user 引数は \p proc() の第二引数として渡される。
 * \p head はリストの本当の先頭要素である必要はない。
 *
 * この関数は返される要素とその前後の要素の next メンバーを修正しない。
 * リストから特定の要素を<b>外す</b>必要があるときは、prefix##Remove() を使うこと。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_PICK(typename, prefix)										\
typename* prefix##Pick(typename* head, int (*proc)(typename*, void*), void* user)		\
{																						\
	typename* p;																		\
																						\
	for(p = head; p; p = STDLIST_NEXT(p)) {												\
		if(proc(p, user)) return p;													\
	}																					\
																						\
	return NULL;																		\
}

/**
 * \p head から始まるリストの末尾に \p node を追加し、追加した \p node を返す
 *
 * \p head から始まるリストを末尾要素までスキャンし、末尾要素の next が \p node を
 * 指すようにする。
 *
 * このとき、<b>\p node の next メンバーを修正しない。</b>つまり、\p node は要素一個
 * でもいいし、\p node からリストが続いていても良い。\p node 以降の要素の next メンバーを
 * あらかじめ正しく設定しておくのはプログラマの責任である。
 *
 * \p node はコピーではなく、参照の修正によってリストに結合される。結合後の \p head を
 * prefix##Free() すると、\p node 以降もまとめて破棄されることになる。
 *
 * \p *head が NULL（つまりリストがカラ）のときは、\p *head が \p node を指すようになる。
 *
 * 同じリストに対して \p node をひとつずつ追加するような場面で、ループの中で
 * この関数を同じ \p head について繰り返し呼ぶのは（毎回伸びていくリストを先頭から
 * 末尾探索しなければならないので）効率が悪い。
 *
 * 単一の \p node を同一の \p head に繰り返し追加するときは prefix##AppendLast()
 * が使えないか検討すること。また追加する順序が問題にならない場合は prefix##Prepend()
 * が使える。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_APPEND(typename, prefix)				\
typename* prefix##Append(typename** head, typename* node)		\
{																\
	typename* tail;												\
																\
	if(*head) {													\
		tail = *head;											\
		if(tail->_next != NULL)									\
			tail = prefix##FindTail(tail);						\
		tail->_next = node;										\
	} else {													\
		*head = node;											\
	}															\
	return node;												\
}

/**
 * リスト \p head の先頭に \p node を挿入し、\p node を返す
 *
 * \p node が一個のときは、\p node の次の要素として \p head を指すようにする。
 * このとき、末尾探索などのリストの走査は発生しないので処理時間は常に O(1) である。
 *
 * \p node がリストの時は、リスト \p node の最終要素の次が \p head になるようにする。
 * このとき末尾探索が発生するので、処理時間はリスト \p node の要素数に比例する
 * （prefix##Append(head, node) の処理時間が \p head の要素数に比例するのと比較せよ）。
 *
 * この関数はリストの新しい先頭要素を返す。これを次の head として呼び出しに使わないと、
 * リストにならないので注意。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_PREPEND(typename, prefix)			\
typename* prefix##Prepend(typename* head, typename* node)		\
{																\
	if(node->_next == NULL) {									\
		node->_next = head;										\
	} else {													\
		typename* tail;											\
		tail = prefix##FindTail(node);							\
		tail->_next = head;										\
	}															\
	return node;												\
}

/**
 * \p head から始まるリストの末尾 \p last に単一要素 \p node を追加し、新たな末尾要素を返す
 *
 * 同一の \p head に対して \p node を一個ずつくり返し追加するような場面で、末尾探索の
 * 回数を減らして処理を高速化する。
 *
 * 初めて呼び出すときは、\p *last に NULL を入れて、そのほかは prefix##Append() と
 * 同じように呼び出す。すると \p *head から始まるリストの末尾に \p node を追加し、
 * \p *last に \p node が入って返ってくる。
 *
 * 次回以降の呼び出しでは、\p *last をそのままにして呼び出す。\p *last が NULL でないとき、
 * この関数は \p node を \p *last の直接の次の要素になるように設定し、\p *last に \p node を入れて返る。
 *
 * このように \p node が単一要素であり、かつ \p *last が末尾要素を指している限り、
 * 末尾探索は最初の一回だけで済むようになる。
 *
 * これら以外の振る舞いについては prefix##Append() と同じである。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_APPEND_LAST(typename, prefix)				\
typename*																\
prefix##AppendLast(typename** head, typename** last, typename* node)	\
{																		\
	if(*head && *last) {												\
		*last = prefix##Append(last, node);								\
	} else {															\
		*last = prefix##Append(head, node);								\
	}																	\
	return *last;														\
}

/**
 * 新しい要素をヒープに割り当て、\p node の内容をコピーして返す。
 *
 * この関数は \p node 一個だけをコピーする。\p node 以降のリストをすべてコピーするには
 *  prefix##DupDeep() を使う。
 *
 * コピーはまず memcpy(3) で行われる。要素にポインタなどがあってただのメモリ
 * コピーでは不十分な場合には、ユーザが定義する prefix##Copy() で独自のコピー
 * 処理を行う。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_DUP_SHALLOW(typename, prefix)	\
typename* prefix##DupShallow(typename* node)				\
{															\
	typename* new_one = prefix##New();						\
															\
	memcpy(new_one, node, sizeof(typename));				\
	prefix##Copy(node, new_one);							\
	return new_one;										\
}

/**
 * \p top 以降のすべての要素をコピーした新しいリストを作って返す
 *
 * この関数は \p top 以降のすべての要素をコピーしたリストを作る。
 *
 * 要素一個あたりのコピーには prefix##DupShallow() を使っている。
 * ユーザは prefix##Copy() を実装するだけで、この関数も使うことができる。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_DUP_DEEP(typename, prefix)	\
typename* prefix##DupDeep(typename* top)				\
{														\
	typename* src;										\
	typename* dst;										\
	typename* d_top;									\
	typename* d_next;									\
														\
	src = top;											\
	dst = d_top = prefix##DupShallow(src);				\
	src = src->_next;									\
														\
	while(src) {										\
		d_next = prefix##DupShallow(src);				\
		dst->_next = d_next;								\
		dst = d_next;									\
		src = src->_next;								\
	}													\
	return d_top;										\
}

/**
 * \p top 以降すべての要素をメモリから解放する
 *
 * 要素が free(3) される前に、ユーザ定義関数 prefix##Destroy() が
 * デストラクタとして呼び出される。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_FREE(typename, prefix)	\
void prefix##Free(typename* top)					\
{													\
	typename* p = top;								\
	typename* q;									\
													\
	while(p) {										\
		q = p->_next;								\
		prefix##Destroy(p);							\
		free(p);									\
		p = q;										\
	}												\
}

/**
 * \p top 以降の要素から \p to_remove を探し、それを取り除いたリストを返す
 *
 * 返されるリストは top 以降を修正したものであり、コピーではない。
 *
 * 要素の比較は単純にポインタの == で行われる。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_REMOVE(typename, prefix)				\
typename* prefix##Remove(typename* top, typename* to_remove)	\
{																\
	typename* p = top;											\
	typename* q;												\
																\
	if(top == to_remove) {										\
		q = top->_next;											\
		to_remove->_next = NULL;								\
		return q;												\
	}															\
																\
	while(p) {													\
		q = p->_next;											\
																\
		if(q == to_remove) {									\
			p->_next = q->_next;								\
			to_remove->_next = NULL;							\
			p = p->_next;										\
		} else {												\
			p = q;												\
		}														\
	}															\
																\
	return top;												\
}

/**
 * \p *top の先頭要素をリストから外して返す。
 *
 * *top には外した先頭要素の次の要素が入る。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_OP_SHIFT(typename, prefix)				\
typename* prefix##Shift(typename** top)							\
{																\
	typename* p = *top;											\
																\
	if(p == NULL) return NULL;									\
																\
	*top = STDLIST_NEXT(p);										\
	STDLIST_NEXT(p) = NULL;										\
																\
	return p;													\
}


/**
 * リスト \p head の各要素へのポインタの配列 \p out を返す。
 *
 * 各要素の next メンバーは修正しない。
 * 配列要素の並びはもとのリストの並びと同じである。
 *
 * 関数の戻り値は要素の数なので、割り当てられるメモリは 個数 * sizeof(typename*)
 * となる。out に返った配列を使い終わったら free(3) すること。コピーはしていないので、
 * 配列の各要素を free(3) する必要はない。
 *
 * \p out の各要素は \p head 以下を単に指しているだけなので、\p out より先に \p head
 * が解放されたりしないように注意すること。
 *
 * \p head が NULL, つまりリストがカラのときは out を変更せずに 0 を返す。
 *
 * この関数の使い道については prefix##Sort() 参照。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_CONV_LIST_TO_ARRAY(typename, prefix)	\
size_t prefix##ListToArray(typename* head, typename*** out)		\
{																\
	typename** array;											\
	size_t count, i;											\
	typename* p;												\
																\
	if(head == NULL) {											\
		return 0;												\
	}															\
																\
	count = prefix##Count(head);								\
	array = calloc(count, sizeof(typename*));					\
																\
	for(p = head, i = 0; p; p = p->_next, i++) {				\
		array[i] = p;											\
	}															\
																\
	*out = array;												\
	return count;												\
}

/**
 * リストの各要素へのポインタの配列 \p array をリストにつなげ直して返す
 *
 * リスト要素の並び順は \p array の要素の並びと同じになる。
 * この関数は各要素の next メンバーを修正する。
 *
 * \p array の要素数は \p count で与える。
 *
 * つなげ直したリストの先頭要素を返す。
 * \p count が 0 のときはなにもしないで NULL を返す。
 *
 * この関数の使い道については prefix##Sort() 参照。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_CONV_ARRAY_TO_LIST(typename, prefix)	\
typename* prefix##ArrayToList(typename** array, size_t count)	\
{																\
	typename* head;												\
	typename* p;												\
	size_t i;													\
																\
	if(count == 0) return NULL;								\
																\
	head = p = array[0];										\
																\
	for(i = 1; i < count; i++) {								\
		p->_next = array[i];									\
		p = array[i];											\
	}															\
	p->_next = NULL;											\
	return head;												\
}

/**
 * リスト \p *head の各要素を引数に \p proc() を呼び出し、真を返した要素を \p *head
 * 以下からとり除いて別のリストにして返す。
 *
 * この結果 \p head には \p proc() が真を返さなかった要素だけが残ることになる。
 * すべての要素について \p proc() が真なら \p *head が NULL になることもある。
 *
 * \p head が NULL なら NULL を返す。\p user 引数は \p proc() の第二引数として渡される。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_FILTER_SELECT(typename, prefix)						 \
typename*																	 \
prefix##Select(typename** head, int (*proc)(typename*, void*), void* user) \
{																			 \
	typename* p;															 \
	typename* q;															 \
	typename* selected = NULL;												 \
	typename* sel_last = NULL;												 \
																			 \
	p = *head;																 \
	q = NULL;																 \
	while(p) {																 \
		if(proc(p, user)) {													 \
			if(q) {															 \
				q->_next = p->_next;										 \
				p->_next = NULL;											 \
				prefix##AppendLast(&selected, &sel_last, p);				 \
				p = q->_next;												 \
			} else {														 \
				*head = p->_next;											 \
				p->_next = NULL;											 \
				prefix##AppendLast(&selected, &sel_last, p);				 \
				p = *head;													 \
			}																 \
		} else {															 \
			q = p;															 \
			p = p->_next;													 \
		}																	 \
	}																		 \
																			 \
	return selected;														 \
}

/**
 * \p *head から始まるリストの隣接する要素を引数に \p proc() を呼び出し、0 を
 * 返したときの後者の要素を \p *head 以下から取り除いて別のリストにして返す。
 *
 * あらかじめ \p *head をソートしておき（prefix##Sort()）、次にこの関数を \p proc()
 * に等価判定をする関数を与えて呼び出すことで、重複する要素をリスト \p head から
 * 除去する目的に使うことができる。
 *
 * 引数 \p user は \p proc() の第三引数に渡される。
 *
 * 例えばリストが <code>a -> b -> c -> d</code> と並んでいたら、\p proc() は
 * <pre>
 *   proc(a, b, user);
 *   proc(b, c, user);
 *   proc(c, d, user);
 * </pre>
 * と呼び出される。
 *
 * \p proc() が 0 を返した時に第二引数として渡された要素だけを集めたリストを作って返す。
 * \p *head が NULL のときは NULL を返す。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_FILTER_UNIQ(typename, prefix)									\
typename*																				\
prefix##Uniq(typename** head, int (*proc)(typename*, typename*, void*), void* user)	\
{																						\
	typename* p;																		\
	typename* q;																		\
	typename* selected = NULL;															\
	typename* sel_last = NULL;															\
																						\
	if(*head == NULL) return NULL;														\
																						\
	p = (*head)->_next;																	\
	q = *head;																			\
	while(p) {																			\
		if(proc(q, p, user) == 0) {														\
			q->_next = p->_next;														\
			p->_next = NULL;															\
			prefix##AppendLast(&selected, &sel_last, p);								\
			p = q->_next;																\
		} else {																		\
			q = p;																		\
			p = p->_next;																\
		}																				\
	}																					\
	return selected;																	\
}

/**
 * \p input から始まるリストを破壊的にソートして返す。
 *
 * 比較関数は cmp() で与える。これは qsort(3) で使う比較関数と同じものである。
 * 比較関数には typename** が渡されるので間違わないこと。
 *
 * この関数から返ると、\p input が先頭要素とは限らなくなる。
 * 関数の戻り値を先頭要素として使うこと。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_FILTER_SORT(typename, prefix)									\
typename* prefix##Sort(typename* input, int (*cmp)(const void*, const void*))		\
{																						\
	size_t items_in_array;																\
	typename** array = NULL;															\
	typename* result;																	\
																						\
	items_in_array = prefix##ListToArray(input, &array);								\
	qsort(array, items_in_array, sizeof(typename*), cmp);								\
	result= prefix##ArrayToList(array, items_in_array);									\
	free(array);																		\
																						\
	return result;																		\
}

/**
 * リスト化した構造体を扱う関数を定義する。
 *
 * ヘッダファイルでは、リスト化したい構造体の定義に DEFINE_STDLIST_MEMBERS() を指定し、
 * 関数プロトタイプを宣言するヘッダファイルで、DECLARE_STDLIST() を呼び出す。
 *
 * ソースファイルでは、DECLARE_STDLIST_USER_FUNCTIONS() で宣言しているユーザ関数を定義したら、
 * このマクロを呼ぶことで、リスト構造に対する操作関数がまとめて定義される。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DEFINE_STDLIST_IMPL(typename, prefix)		\
DEFINE_STDLIST_OP_NEW(typename, prefix)				\
DEFINE_STDLIST_OP_FINDTAIL(typename, prefix)		\
DEFINE_STDLIST_OP_COUNT(typename, prefix)			\
DEFINE_STDLIST_OP_COUNT_IF(typename, prefix)		\
DEFINE_STDLIST_OP_PICK(typename, prefix)			\
DEFINE_STDLIST_OP_PREPEND(typename, prefix)			\
DEFINE_STDLIST_OP_APPEND(typename, prefix)			\
DEFINE_STDLIST_OP_APPEND_LAST(typename, prefix)		\
DEFINE_STDLIST_OP_DUP_SHALLOW(typename, prefix)		\
DEFINE_STDLIST_OP_DUP_DEEP(typename, prefix)		\
DEFINE_STDLIST_OP_FREE(typename, prefix)			\
DEFINE_STDLIST_OP_REMOVE(typename, prefix)			\
DEFINE_STDLIST_OP_SHIFT(typename, prefix)			\
DEFINE_STDLIST_CONV_LIST_TO_ARRAY(typename, prefix)	\
DEFINE_STDLIST_CONV_ARRAY_TO_LIST(typename, prefix)	\
DEFINE_STDLIST_FILTER_SELECT(typename, prefix)		\
DEFINE_STDLIST_FILTER_UNIQ(typename, prefix)		\
DEFINE_STDLIST_FILTER_SORT(typename, prefix)


#endif /* STDLIST_IMPL_H_ */
