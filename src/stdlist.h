/** @file stdlist.h
 * 汎用線形リストメタプログラミングライブラリ
 *
 * マクロ展開によるメタプログラミングによって、
 * 少ない手間で構造体を単方向リストにし、また豊富な操作関数を提供する。
 *
 * \section 型の宣言
 *
 * リストにしたい構造体を定義しているヘッダファイルで stdlist.h を \#include する。
 *
 * リストにしたい構造体定義の最後に、 DEFINE_STDLIST_MEMBERS() を、リストにしたい構造体の型名を引数に呼び出す。
 * \code
 * struct footype {
 *   int foo;
 *   char bar[16];
 *   char* buffer;
 *
 *   DEFINE_STDLIST_MEMBERS(struct footype)
 * };
 * \endcode
 * 構造体定義と同時に typedef しているときはタグ名（struct 何々）のほうで指定する。
 * あらかじめ構造体の名前だけの宣言を typedef してあれば（typedef struct foo;）、
 * typedef 名で指定できる。
 *
 * \section プロトタイプの宣言
 *
 * ヘッダファイルの関数プロトタイプ宣言をする文脈で、 DECLARE_STDLIST() を呼び出す。
 * \code
 * struct footype {
 *   int foo;
 *   char bar[16];
 *   char* buffer;
 *
 *   DEFINE_STDLIST_MEMBERS(struct footype)
 * };
 *
 * DECLARE_STDLIST(struct footype, foo)
 * \endcode
 * このマクロの第一引数は DEFINE_STDLIST_MEMBERS() と同じ型名で、第二引数は
 * この構造体を操作する関数に付けるプリフィクスを指定する。たとえば
 * <code>DECLARE_STDLIST(struct footype, foo)</code> と宣言すると、操作関数のプロトタイプは以下のようになる。
 * \code
 * struct footype* fooAppend(struct footype** head, struct footype* node);
 * \endcode
 *
 * \section ユーザ定義関数
 *
 * 以下はソースファイルで行う。stdlist.h と stdlist_impl.h を \#include する。
 *
 * リストライブラリの実装上必要な 3 つの関数を実装する。必要な関数は以下のとおり。
 * \code
 *  void prefix##Init(typename* p);
 *  void prefix##Copy(typename* from, typename* to);
 *  void prefix##Destroy(typename* p);
 * \endcode
 *
 * 例にあげている DECLARE_STDLIST(struct footype, foo) では、以下のようなプロトタイプになる。
 *
 * <dl>
 * <dt>void fooInit(struct footype* p)</dt>
 * <dd>
 * メモリに確保した構造体 \p p を初期化する。メモリの確保には calloc(3) を使っているので
 * ゼロクリアは行われているが、構造体に固有の初期化が必要なときにはこの関数でそれを実装する。
 * </dd>
 *
 * <dt>void fooCopy(struct footype* from, struct footype* to)</dt>
 * <dd>\p from を \p to にコピーする。この関数が呼ばれる前に memcpy(3) しているので、
 * \p to の各メンバーには \p from と同じ値が入っているが、メンバーにポインタがある
 * などしたときに深いコピーを作る必要があるときはこの関数で行う。</dd>
 *
 * <dt>void fooDestroy(struct footype* p)</dt>
 * <dd>\p p をメモリから解放する直前に呼ばれる。メンバーが何らかのリソースを掴んでいたり、
 * malloc(3) したメモリを指しているときなどにはここで解放する必要がある。
 * </dd>
 * </dl>
 *
 * これらはメンバーに解放責任のあるポインタやディスクリプタなどがあるときには
 * 中身を書かなければならないが、そうでないのならば中に何も書く必要はないはずである。
 *
 * \section 操作関数の定義
 *
 * これらの関数を実装したら、ソースファイルの最後に
 * \code
 * DEFINE_STDLIST_IMPL(struct footype, foo)
 * \endcode
 * と記述する。これで stdlist_impl.h に定義したマクロが展開され、struct foo 用の
 * 操作関数が定義される。
 *
 * \section ライブラリを使う
 *
 * ライブラリを使う側は、構造体を定義したヘッダファイルを \#include し、リンクするときに
 * 関数定義を含むソースファイルをリンクする。
 *
 * @date 2011/01/17
 */

#ifndef STDLIST_H_
#define STDLIST_H_

#include <stdlib.h>

/**
 * 構造体をリストにするために必要なメンバーを宣言する。
 *
 * リストにしたい構造体の定義にこの呼び出しを加えることで、リストの実装
 * に必要な要素を定義に追加する。
 *
 * \code
 * struct foo {
 *   int foo;
 *   long bar;
 *   DEFINE_STDLIST_MEMBERS(struct foo)
 * };
 * \endcode
 *
 * @param typename リストにしたい構造体の型名。typedef された名前でも良
 * いが、その場合はその名前は構造体定義の前に宣言されていなければならな
 * い。これは自己参照のある型には共通の制約である。
 */
#define DEFINE_STDLIST_MEMBERS(typename) \
	typename* _next;

/**
 * リストの「次の要素へのポインタ」を返す
 *
 * たとえばリストの先頭要素 head 以下全要素をループで回すようなときは、
 * 次のようにかける：
 * \code
 *   for(i = head; i; i = STDLIST_NEXT(i)) {
 *     // code here...
 *   }
 * \endcode
 *
 * 左辺値として使うこともできる。たとえばリスト要素 p をリストの最後の
 * 要素にするには：
 * \code
 *   STDLIST_NEXT(p) = NULL;
 * \endcode
 *
 * @param p リストの要素へのポインタ。
 * @return リスト上で p の次の要素へのポインタを返す
 */
#define STDLIST_NEXT(p)	((p)->_next)

/**
 * リストの head 以降の各要素を iter に代入しながらループする
 *
 * \code
 * for(iter = head; iter; iter = STDLIST_NEXT(iter))
 * \endcode
 *
 * のショートカットである。このマクロに文を続けて書くことで制御構造のように使える。
 *
 * @param head リストの先頭要素
 * @param iter イテレータとして使う変数名
 */
#define STDLIST_FOREACH(head, iter)	for((iter) = (head); (iter); (iter) = STDLIST_NEXT((iter)))

/**
 * 構造体をリスト化するにあたってユーザが定義しなければならない関数のプ
 * ロトタイプを宣言する。
 *
 * コンストラクタ・デストラクタ・コピーコンストラクタに相当する関数を定
 * 義することで、メンバーにポインタを含むような構造体のリスト化を簡単に
 * 実現する。
 *
 * 通常は DECLARE_STDLIST() でまとめて宣言されるので、
 * このマクロを手で呼び出す必要はない。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DECLARE_STDLIST_USER_FUNCTIONS(typename, prefix)	\
void prefix##Init(typename* p);							\
void prefix##Copy(typename* from, typename* to);			\
void prefix##Destroy(typename* p);

/**
 * リスト化した構造体に対する基本的な操作関数を宣言する。
 *
 * 通常は DECLARE_STDLIST() でまとめて宣言されるので、
 * このマクロを手で呼び出す必要はない。
 *
 * @internal
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DECLARE_STDLIST_OPERATORS(typename, prefix)								\
typename*	prefix##New(void);														\
typename*	prefix##FindTail(typename* head);										\
size_t		prefix##Count(typename* head);											\
size_t		prefix##CountIf(typename* head, int (*proc)(typename*, void*), void* user); \
typename*	prefix##Pick(typename* head, int (*proc)(typename*, void*), void* user);	\
typename*	prefix##Prepend(typename* head, typename* node);						\
typename*	prefix##Append(typename** head, typename* node);						\
typename*	prefix##AppendLast(typename** head, typename** last, typename* node);	\
typename*	prefix##DupShallow(typename* result);									\
typename*	prefix##DupDeep(typename* top);											\
void		prefix##Free(typename* top);											\
typename*	prefix##Remove(typename* top, typename* to_remove);						\
typename*	prefix##Shift(typename** top);

/**
 * リスト化した構造体と、その構造体の配列との相互変換を行う関数を宣言する。
 *
 * 通常は DECLARE_STDLIST() でまとめて宣言されるので、
 * このマクロを手で呼び出す必要はない。
 *
 * @internal
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DECLARE_STDLIST_CONVERTERS(typename, prefix)								\
size_t		prefix##ListToArray(typename* head, typename*** out);					\
typename*	prefix##ArrayToList(typename** array, size_t count);					\
typename*	prefix##Sort(typename* input, int (*cmp)(const void*, const void*));

/**
 * リスト化した構造体全体に対してフィルタのように振る舞う関数を宣言する。
 *
 * 通常は DECLARE_STDLIST() でまとめて宣言されるので、
 * このマクロを手で呼び出す必要はない。
 *
 * @internal
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DECLARE_STDLIST_FILTERS(typename, prefix)											\
typename*	prefix##Select(typename** head, int (*proc)(typename*, void*), void* user); 	\
typename*	prefix##Uniq(typename** head, int (*proc)(typename*, typename*, void*), void* user);


/**
 * リスト化した構造体に対する関数プロトタイプをまとめて宣言する。
 *
 * ヘッダファイルでは、リスト化したい構造体の定義に DEFINE_STDLIST_MEMBERS() を指定し、
 * 関数プロトタイプを宣言するヘッダファイルで、このマクロを呼び出す。
 *
 * ソースファイルでは、 DECLARE_STDLIST_USER_FUNCTIONS() で宣言しているユーザ関数を定義したら、
 * DEFINE_STDLIST_IMPL() を呼ぶことで、リスト構造に対する操作関数がまとめて定義される。
 *
 * @param typename リスト化する構造体の型名を指定する。
 * @param prefix 関数名につけるプレフィックスを指定する。
 */
#define DECLARE_STDLIST(typename, prefix)			\
DECLARE_STDLIST_USER_FUNCTIONS(typename, prefix)	\
DECLARE_STDLIST_OPERATORS(typename, prefix)			\
DECLARE_STDLIST_CONVERTERS(typename, prefix)		\
DECLARE_STDLIST_FILTERS(typename, prefix)

#endif /* STDLIST_H_ */
