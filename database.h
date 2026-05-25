#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

/*==========================================================================
 * 状态码枚举 —— 统一管理所有操作的返回值语义
 *==========================================================================*/
typedef enum {
    STATUS_OK              =  0,  // 操作成功
    STATUS_EXISTS          =  1,  // 资源已存在（用户名/收藏重复）
    STATUS_OUT_OF_STOCK    =  2,  // 库存不足
    STATUS_NOT_FOUND       = -1,  // 资源未找到
    STATUS_DB_ERROR        = -2,  // 数据库操作失败
    STATUS_INVALID_PARAM   = -3   // 参数不完整/非法
} StatusCode;

/*==========================================================================
 * 图书交易状态枚举
 *==========================================================================*/
typedef enum {
    BOOK_STATUS_ON_SALE = 0,  // 在售
    BOOK_STATUS_SOLD    = 1,  // 已售
    BOOK_STATUS_DELISTED = 2  // 已下架
} BookStatus;

/*==========================================================================
 * 数据结构 —— 链表节点定义
 *==========================================================================*/

// 用户信息
typedef struct User {
    int    id;
    char   username[64];
    char   password[128];
    int    is_profile_complete;
    struct User *next;               // 链表指针
} User;

// 用户详细资料
typedef struct UserProfile {
    int    user_id;
    char   name[64];
    char   class_name[64];
    char   student_id[32];
    char   avatar[256];
    struct UserProfile *next;
} UserProfile;

// 图书信息（链表节点）—— 扩展字段以匹配设计文档要求
typedef struct Book {
    int    id;
    char   name[128];           // 书名
    char   author[128];         // 作者
    char   isbn[32];            // ISBN/书号
    char   publisher[128];      // 出版社
    char   category[64];        // 分类
    char   condition[32];       // 新旧程度
    float  price;               // 售价
    int    stock;               // 库存
    int    status;              // 交易状态：0-在售 1-已售 2-已下架
    int    user_id;             // 卖家用户ID
    char   seller_name[64];     // 卖家姓名
    char   image_url[256];      // 图片URL
    char   create_time[32];     // 发布时间（YYYY-MM-DD HH:MM:SS）
    struct Book *next;          // 链表指针
} Book;

// 购买记录
typedef struct Purchase {
    int    id;
    int    user_id;             // 买家ID
    int    book_id;
    char   buyer_name[64];      // 买家姓名
    char   transaction_time[32];// 交易时间
    struct Purchase *next;
} Purchase;

// 统计信息
typedef struct Stats {
    int    total_listed;        // 总上架数量
    int    total_sold;          // 已售数量
    int    total_delisted;      // 下架数量
    float  total_amount;        // 总交易金额
    int    category_count;      // 分类统计项数
    char   categories[32][64];  // 分类名称列表
    int    category_sold[32];   // 各分类已售数量
} Stats;

/*==========================================================================
 * 函数指针类型 —— 用于路由分发表
 *==========================================================================*/

// 路由处理函数：接收 JSON 请求体，返回 JSON 响应字符串（由调用者释放）
typedef char* (*RouteHandler)(const char *body);

// 带 URL 参数的路由处理函数
typedef char* (*RouteHandlerWithID)(int id, const char *query_params);

/*==========================================================================
 * 路由条目 —— 将路径、方法、处理函数绑定在一起
 *==========================================================================*/
typedef struct Route {
    const char  *path;               // 接口路径，如 "/api/books"
    const char  *method;             // 请求方法，"GET" 或 "POST"
    RouteHandler handler;            // 处理函数
} Route;

/*==========================================================================
 * 链表辅助宏 / 内联函数
 *==========================================================================*/

// 安全释放整个链表
#define FREE_LIST(head, type)  do {           \
    type *_p = (head), *_tmp;                 \
    while (_p) { _tmp = _p->next; free(_p); _p = _tmp; } \
    (head) = NULL;                            \
} while(0)

/*==========================================================================
 * 接口声明
 *==========================================================================*/

// 数据库生命周期
int  init_database(void);
void close_database(void);

// 用户操作
int  register_user(const char *json_data);
int  login_user(const char *json_data, User **out_user);
int  get_user_profile(int user_id, UserProfile **out_profile);
int  update_user_profile(const char *json_data);

// 图书操作
int  add_book(const char *json_data);
int  get_all_books(Book **out_books, int *out_count);
int  get_book_by_id(int book_id, Book **out_book);

// 图书搜索（多条件查询）
int  search_books(const char *keyword, const char *author,
                  const char *isbn, float min_price, float max_price,
                  int seller_id, int status, const char *category,
                  Book **out_books, int *out_count);

// 收藏操作
int  add_collect(const char *json_data);
int  get_collect_books(int user_id, Book **out_books, int *out_count);

// 发布与购买
int  get_published_books(int user_id, Book **out_books, int *out_count);
int  get_purchased_books(int user_id, Book **out_books, int *out_count);
int  purchase_book(const char *json_data);

// 下架图书
int  delist_book(int book_id, int user_id);

// 统计报表
int  get_stats(Stats **out_stats);

#endif /* DATABASE_H */