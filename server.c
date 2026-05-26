#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <winsock2.h>
#include <windows.h>
#include "database.h"

/*
 * 校园二手书交易管理系统 - HTTP 服务器
 *
 * 功能：
 *   - 用户注册/登录/资料管理
 *   - 图书发布、购买、下架
 *   - 多条件图书搜索（按书名、作者、ISBN、价格区间、状态等）
 *   - 收藏管理
 *   - 统计报表
 *
 * 端口：8080
 * 数据格式：JSON
 */

/*==========================================================================
 * 常量与全局变量
 *==========================================================================*/

#define PORT            8080
#define BUFFER_SIZE     65536
#define MAX_BOOKS_JSON  (BUFFER_SIZE / 2)

/*==========================================================================
 * 辅助函数
 *==========================================================================*/

/* URL 解码（将 %XX 转换为原始字符） */
static void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            a = src[1]; b = src[2];
            *dst++ = (char)((a >= 'A' ? ((a & 0xDF) - 'A' + 10) : (a - '0')) << 4
                          | (b >= 'A' ? ((b & 0xDF) - 'A' + 10) : (b - '0')));
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/* 从 URL 查询字符串中提取参数值 */
static const char* extract_query_param(const char *query, const char *key,
                                       char *out, size_t size) {
    char search[64];
    snprintf(search, sizeof(search), "%s=", key);
    const char *pos = strstr(query, search);
    if (!pos) { out[0] = '\0'; return out; }
    pos += strlen(search);
    size_t i = 0;
    while (*pos && *pos != '&' && i < size - 1) out[i++] = *pos++;
    out[i] = '\0';

    // URL 解码
    char decoded[512];
    url_decode(decoded, out);
    strcpy(out, decoded);
    return out;
}

/* 从请求 URL 中提取 userId 参数 */
static int url_get_userid(const char *request) {
    const char *pos = strstr(request, "userId=");
    if (!pos) {
        pos = strstr(request, "userid=");
        if (!pos) return 0;
    }
    pos += 7;
    return atoi(pos);
}

/* 发送 HTTP 响应头和 JSON 体 */
static void send_json(SOCKET sock, const char *json) {
    char response[BUFFER_SIZE];
    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s", (int)strlen(json), json);
    send(sock, response, len, 0);
}

/* 发送静态 HTML 文件 */
static void send_html(SOCKET sock, const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        const char *not_found =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Connection: close\r\n\r\n"
            "<h1>404 - 页面未找到</h1>";
        send(sock, not_found, (int)strlen(not_found), 0);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char header[256];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n", fsize);
    send(sock, header, hlen, 0);

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
        send(sock, buf, (int)n, 0);
    fclose(fp);
}

/* 发送 CORS 预检响应 */
static void send_cors(SOCKET sock) {
    const char *res =
        "HTTP/1.1 204 No Content\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n\r\n";
    send(sock, res, (int)strlen(res), 0);
}

/* 将 Book 链表转换为 JSON 数组字符串 */
static char* book_list_to_json(Book *head, int count) {
    if (!head) {
        char *res = (char*)malloc(64);
        if (res) snprintf(res, 64, "{\"code\": 200, \"data\": [], \"total\": 0}");
        return res;
    }

    char *buf = (char*)malloc(MAX_BOOKS_JSON);
    if (!buf) return NULL;
    char *p = buf;
    const char *end = buf + MAX_BOOKS_JSON - 1;

    p += snprintf(p, end - p,
                  "{\"code\": 200, \"data\": [");

    Book *b = head;
    int first = 1;
    while (b && p < end) {
        if (!first) { *p++ = ','; }
        first = 0;

        // 状态名称映射
        const char *status_name = "在售";
        if (b->status == 1) status_name = "已售";
        else if (b->status == 2) status_name = "已下架";

        p += snprintf(p, end - p,
            "{"
            "\"id\": %d, "
            "\"name\": \"%s\", "
            "\"author\": \"%s\", "
            "\"isbn\": \"%s\", "
            "\"publisher\": \"%s\", "
            "\"category\": \"%s\", "
            "\"condition\": \"%s\", "
            "\"price\": %.2f, "
            "\"stock\": %d, "
            "\"status\": %d, "
            "\"statusName\": \"%s\", "
            "\"userId\": %d, "
            "\"sellerName\": \"%s\", "
            "\"images\": [\"%s\"], "
            "\"createTime\": \"%s\""
            "}",
            b->id, b->name, b->author, b->isbn, b->publisher,
            b->category, b->condition, b->price, b->stock,
            b->status, status_name,
            b->user_id, b->seller_name, b->image_url, b->create_time);
        b = b->next;
    }

    p += snprintf(p, end - p, "], \"total\": %d}", count);
    return buf;
}

/*==========================================================================
 * POST 请求体处理函数
 *==========================================================================*/

static char* handle_register(const char *body) {
    char *res = (char*)malloc(128);
    int rc = register_user(body);
    snprintf(res, 128, "{\"code\": %d, \"message\": \"%s\"}",
             rc == STATUS_OK ? 200 : (rc == STATUS_EXISTS ? 409 : 500),
             rc == STATUS_OK ? "注册成功" :
             rc == STATUS_EXISTS ? "用户名已存在" : "注册失败");
    return res;
}

static char* handle_login(const char *body) {
    char *res = (char*)malloc(512);
    User *user = NULL;
    int rc = login_user(body, &user);
    if (rc == STATUS_OK || rc == STATUS_CREATED) {
        /* STATUS_OK: 已有用户验证通过；STATUS_CREATED: 新用户自动注册 */
        snprintf(res, 512,
                 "{\"code\": 200, \"message\": \"%s\", "
                 "\"data\": {\"id\": %d, \"username\": \"%s\", \"isProfileComplete\": %d}}",
                 rc == STATUS_CREATED ? "注册并登录成功" : "登录成功",
                 user->id, user->username, user->is_profile_complete);
        free(user);
    } else {
        snprintf(res, 256, "{\"code\": 401, \"message\": \"用户名或密码错误\"}");
    }
    return res;
}

static char* handle_profile_update(const char *body) {
    char *res = (char*)malloc(128);
    int rc = update_user_profile(body);
    snprintf(res, 128, "{\"code\": %d, \"message\": \"%s\"}",
             rc == STATUS_OK ? 200 : 500,
             rc == STATUS_OK ? "更新成功" : "更新失败");
    return res;
}

static char* handle_books_get(const char *body) {
    (void)body;
    Book *head = NULL;
    int count = 0;
    get_all_books(&head, &count);
    char *res = book_list_to_json(head, count);
    FREE_LIST(head, Book);
    return res;
}

static char* handle_books_post(const char *body) {
    char *res = (char*)malloc(256);
    int book_id = add_book(body);
    snprintf(res, 256,
             "{\"code\": %d, \"message\": \"%s\", \"data\": {\"id\": %d}}",
             book_id > 0 ? 200 : 500,
             book_id > 0 ? "发布成功" : "发布失败",
             book_id);
    return res;
}

static char* handle_collect_post(const char *body) {
    char *res = (char*)malloc(128);
    int rc = add_collect(body);
    snprintf(res, 128, "{\"code\": %d, \"message\": \"%s\"}",
             rc == STATUS_OK ? 200 :
             rc == STATUS_EXISTS ? 409 : 500,
             rc == STATUS_OK ? "收藏成功" :
             rc == STATUS_EXISTS ? "已收藏过" : "收藏失败");
    return res;
}

static char* handle_purchase(const char *body) {
    char *res = (char*)malloc(256);
    int rc = purchase_book(body);
    snprintf(res, 256, "{\"code\": %d, \"message\": \"%s\"}",
             rc == STATUS_OK ? 200 :
             rc == STATUS_OUT_OF_STOCK ? 400 :
             rc == STATUS_NOT_FOUND ? 404 : 500,
             rc == STATUS_OK ? "购买成功" :
             rc == STATUS_OUT_OF_STOCK ? "库存不足或已下架" :
             rc == STATUS_NOT_FOUND ? "图书不在售" : "购买失败");
    return res;
}

/* 下架图书 */
static char* handle_delist(const char *body) {
    char *res = (char*)malloc(256);
    char uid_str[16] = {0}, bid_str[16] = {0}, student_id[32] = {0};
    /* body 格式: {"userId":1, "bookId":5, "studentId":"2021001"} */
    if (body && strstr(body, "userId")) {
        /* 从 JSON body 解析 */
        const char *pos;
        pos = strstr(body, "\"userId\":");
        if (pos) {
            pos += 9;
            int i = 0;
            while (*pos >= '0' && *pos <= '9' && i < 15) uid_str[i++] = *pos++;
            uid_str[i] = '\0';
        }
        pos = strstr(body, "\"bookId\":");
        if (pos) {
            pos += 9;
            int i = 0;
            while (*pos >= '0' && *pos <= '9' && i < 15) bid_str[i++] = *pos++;
            bid_str[i] = '\0';
        }
        /* 提取学号（用于核对发布者身份） */
        pos = strstr(body, "\"studentId\":\"");
        if (pos) {
            pos += 14;
            int i = 0;
            while (*pos && *pos != '"' && i < 31) student_id[i++] = *pos++;
            student_id[i] = '\0';
        }
    }
    int user_id = atoi(uid_str);
    int book_id = atoi(bid_str);
    if (user_id <= 0 || book_id <= 0) {
        snprintf(res, 256, "{\"code\": 400, \"message\": \"参数不完整，需要userId和bookId\"}");
        return res;
    }
    int rc = delist_book(book_id, user_id, student_id);
    snprintf(res, 256, "{\"code\": %d, \"message\": \"%s\"}",
             rc == STATUS_OK ? 200 :
             rc == STATUS_INVALID_PARAM ? 403 :
             rc == STATUS_NOT_FOUND ? 404 : 500,
             rc == STATUS_OK ? "下架成功" :
             rc == STATUS_INVALID_PARAM ? "学号验证失败，无权操作此图书" :
             rc == STATUS_NOT_FOUND ? "图书不在售，无法下架" : "下架失败");
    return res;
}

/*==========================================================================
 * 特殊路由处理（GET 带参数 + 动态路径）
 *==========================================================================*/

/* 获取个人资料 */
static void do_profile_get(SOCKET sock, const char *request) {
    char *res = (char*)malloc(512);
    int uid = url_get_userid(request);
    if (uid <= 0) {
        snprintf(res, 512, "{\"code\": 400, \"message\": \"缺少 userId 参数\"}");
    } else {
        UserProfile *profile = NULL;
        if (get_user_profile(uid, &profile) == STATUS_OK) {
            snprintf(res, 512,
                     "{\"code\": 200, \"data\": {"
                     "\"name\": \"%s\", \"class\": \"%s\", "
                     "\"studentId\": \"%s\", \"avatar\": \"%s\"}}",
                     profile->name, profile->class_name,
                     profile->student_id, profile->avatar);
            free(profile);
        } else {
            snprintf(res, 512, "{\"code\": 404, \"message\": \"未找到用户资料\"}");
        }
    }
    send_json(sock, res);
    free(res);
}

/* 获取收藏/已发布/已购买 列表 */
static void do_book_list_for_user(SOCKET sock, const char *request,
                                  int (*getter)(int, Book**, int*)) {
    int uid = url_get_userid(request);
    if (uid <= 0) {
        send_json(sock, "{\"code\": 400, \"message\": \"缺少 userId 参数\"}");
        return;
    }
    Book *head = NULL;
    int count = 0;
    getter(uid, &head, &count);
    char *json = book_list_to_json(head, count);
    send_json(sock, json ? json : "{\"code\": 500, \"message\": \"内存不足\"}");
    free(json);
    FREE_LIST(head, Book);
}

/* 获取单个图书详情 */
static void do_book_detail(SOCKET sock, const char *path) {
    char *res = (char*)malloc(2048);
    int book_id = atoi(path + 11);  // 跳过 "/api/books/"
    Book *book = NULL;
    if (get_book_by_id(book_id, &book) == STATUS_OK) {
        const char *status_name = "在售";
        if (book->status == 1) status_name = "已售";
        else if (book->status == 2) status_name = "已下架";

        snprintf(res, 2048,
                 "{\"code\": 200, \"data\": {"
                 "\"id\": %d, \"name\": \"%s\", \"author\": \"%s\", "
                 "\"isbn\": \"%s\", \"publisher\": \"%s\", "
                 "\"category\": \"%s\", \"condition\": \"%s\", "
                 "\"price\": %.2f, \"stock\": %d, "
                 "\"status\": %d, \"statusName\": \"%s\", "
                 "\"userId\": %d, \"sellerName\": \"%s\", "
                 "\"images\": [\"%s\"], \"createTime\": \"%s\"}}",
                 book->id, book->name, book->author,
                 book->isbn, book->publisher,
                 book->category, book->condition,
                 book->price, book->stock,
                 book->status, status_name,
                 book->user_id, book->seller_name,
                 book->image_url, book->create_time);
        free(book);
    } else {
        snprintf(res, 2048, "{\"code\": 404, \"message\": \"图书不存在\"}");
    }
    send_json(sock, res);
    free(res);
}

/* 多条件图书搜索 —— 对应设计文档 F. 图书信息查询 */
static void do_book_search(SOCKET sock, const char *request) {
    /* 从 URL 查询字符串中提取参数 */
    const char *query = strchr(request, '?');
    if (!query) {
        /* 无查询参数，返回全部 */
        Book *head = NULL;
        int count = 0;
        get_all_books(&head, &count);
        char *json = book_list_to_json(head, count);
        send_json(sock, json ? json : "{\"code\": 500, \"message\": \"内存不足\"}");
        free(json);
        FREE_LIST(head, Book);
        return;
    }
    query++; /* 跳过 '?' */

    char keyword[128] = {0}, author[64] = {0}, isbn[32] = {0};
    char min_price_str[32] = {0}, max_price_str[32] = {0};
    char seller_id_str[16] = {0}, seller_student_id[32] = {0};
    char status_str[16] = {0}, category[64] = {0};

    extract_query_param(query, "keyword",   keyword,   sizeof(keyword));
    extract_query_param(query, "author",    author,    sizeof(author));
    extract_query_param(query, "isbn",      isbn,      sizeof(isbn));
    extract_query_param(query, "minPrice",  min_price_str, sizeof(min_price_str));
    extract_query_param(query, "maxPrice",  max_price_str, sizeof(max_price_str));
    extract_query_param(query, "sellerId",  seller_id_str, sizeof(seller_id_str));
    extract_query_param(query, "sellerStudentId", seller_student_id, sizeof(seller_student_id));
    extract_query_param(query, "status",    status_str, sizeof(status_str));
    extract_query_param(query, "category",  category,  sizeof(category));

    float min_price = (float)atof(min_price_str);
    float max_price = (float)atof(max_price_str);
    int seller_id = atoi(seller_id_str);
    int status = (status_str[0] != '\0') ? atoi(status_str) : -1;

    Book *head = NULL;
    int count = 0;
    search_books(keyword, author, isbn, min_price, max_price,
                 seller_id, seller_student_id, status, category, &head, &count);
    char *json = book_list_to_json(head, count);
    send_json(sock, json ? json : "{\"code\": 500, \"message\": \"内存不足\"}");
    free(json);
    FREE_LIST(head, Book);
}

/* 下架图书（从URL路径解析） */
static void do_delist_from_url(SOCKET sock, const char *request, const char *path) {
    /* 路径格式: /api/books/:id/delist?userId=1&studentId=2021001 */
    char *res = (char*)malloc(256);
    /* 提取 book_id */
    int book_id = atoi(path + 11);
    int user_id = url_get_userid(request);

    /* 从 URL 查询参数中提取学号 */
    const char *query = strchr(request, '?');
    char student_id[32] = {0};
    if (query) {
        /* 简单提取 studentId 参数 */
        const char *pos = strstr(query, "studentId=");
        if (pos) {
            pos += 10; /* 跳过 "studentId=" */
            int i = 0;
            while (*pos && *pos != '&' && *pos != ' ' && *pos != '\r'
                   && *pos != '\n' && i < 31) {
                student_id[i++] = *pos++;
            }
            student_id[i] = '\0';
        }
    }

    if (book_id <= 0 || user_id <= 0) {
        snprintf(res, 256, "{\"code\": 400, \"message\": \"参数不完整\"}");
        send_json(sock, res);
        free(res);
        return;
    }

    int rc = delist_book(book_id, user_id, student_id);
    snprintf(res, 256, "{\"code\": %d, \"message\": \"%s\"}",
             rc == STATUS_OK ? 200 :
             rc == STATUS_INVALID_PARAM ? 403 :
             rc == STATUS_NOT_FOUND ? 404 : 500,
             rc == STATUS_OK ? "下架成功" :
             rc == STATUS_INVALID_PARAM ? "学号验证失败，无权操作此图书" :
             rc == STATUS_NOT_FOUND ? "图书不在售，无法下架" : "下架失败");
    send_json(sock, res);
    free(res);
}

/* 统计报表 —— 对应设计文档 G. 统计与输出 */
static void do_stats(SOCKET sock) {
    Stats *s = NULL;
    if (get_stats(&s) != STATUS_OK || !s) {
        send_json(sock, "{\"code\": 500, \"message\": \"获取统计数据失败\"}");
        return;
    }

    char *buf = (char*)malloc(4096);
    if (!buf) {
        free(s);
        send_json(sock, "{\"code\": 500, \"message\": \"内存不足\"}");
        return;
    }

    char *p = buf;
    const char *end = buf + 4096 - 1;

    p += snprintf(p, end - p,
        "{\"code\": 200, \"data\": {"
        "\"totalListed\": %d, "
        "\"totalSold\": %d, "
        "\"totalDelisted\": %d, "
        "\"totalAmount\": %.2f, "
        "\"categories\": [",
        s->total_listed, s->total_sold, s->total_delisted, s->total_amount);

    for (int i = 0; i < s->category_count && p < end; i++) {
        if (i > 0) *p++ = ',';
        p += snprintf(p, end - p,
            "{\"name\": \"%s\", \"sold\": %d}",
            s->categories[i], s->category_sold[i]);
    }

    p += snprintf(p, end - p, "]}}");

    send_json(sock, buf);
    free(buf);
    free(s);
}

/*==========================================================================
 * 路由分发表
 *==========================================================================*/

static const Route g_routes[] = {
    { "/api/users/register", "POST", handle_register },
    { "/api/users/login",    "POST", handle_login    },
    { "/api/users/profile",  "POST", handle_profile_update },
    { "/api/books",          "GET",  handle_books_get  },
    { "/api/books",          "POST", handle_books_post },
    { "/api/collect",        "POST", handle_collect_post },
    { "/api/purchase",       "POST", handle_purchase },
    { "/api/books/delist",   "POST", handle_delist },  /* 通过 POST body 下架 */
};

static const int g_route_count = sizeof(g_routes) / sizeof(g_routes[0]);

/*==========================================================================
 * HTTP 请求解析与分发
 *==========================================================================*/

static void handle_request(SOCKET sock, const char *request) {
    /* 解析方法 */
    char method[8] = {0};
    sscanf(request, "%7s", method);

    /* CORS 预检请求 */
    if (strcmp(method, "OPTIONS") == 0) {
        send_cors(sock);
        return;
    }

    /* 提取路径 */
    char path[512] = {0};
    {
        const char *p = request;
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
        size_t i = 0;
        while (*p && *p != ' ' && *p != '?' && i < sizeof(path) - 1)
            path[i++] = *p++;
        path[i] = '\0';
    }

    /* 根路径 → 静态首页 */
    if (strcmp(path, "/") == 0) {
        send_html(sock, "login.html");
        return;
    }

    /* 新闻路径 /news → /mydata.html */
    if (strcmp(path, "/news") == 0) {
        send_html(sock, "mydata.html");
        return;
    }

    /* 静态 HTML 文件 */
    if (strstr(path, ".html") || strstr(path, ".htm") ||
        strstr(path, ".css") || strstr(path, ".js") ||
        strstr(path, ".png") || strstr(path, ".jpg") ||
        strstr(path, ".ico")) {
        /* 去掉开头的 '/' */
        send_html(sock, path + 1);
        return;
    }

    /* ===== 特殊路由处理（带参数或动态路径） ===== */

    /* GET /api/users/profile?userId=X */
    if (strcmp(path, "/api/users/profile") == 0 && strcmp(method, "GET") == 0) {
        do_profile_get(sock, request);
        return;
    }

    /* GET /api/books/collect?userId=X */
    if (strcmp(path, "/api/books/collect") == 0 && strcmp(method, "GET") == 0) {
        do_book_list_for_user(sock, request, get_collect_books);
        return;
    }

    /* GET /api/books/published?userId=X */
    if (strcmp(path, "/api/books/published") == 0 && strcmp(method, "GET") == 0) {
        do_book_list_for_user(sock, request, get_published_books);
        return;
    }

    /* GET /api/books/purchased?userId=X */
    if (strcmp(path, "/api/books/purchased") == 0 && strcmp(method, "GET") == 0) {
        do_book_list_for_user(sock, request, get_purchased_books);
        return;
    }

    /* GET /api/books/search?... —— 多条件查询 */
    if (strcmp(path, "/api/books/search") == 0 && strcmp(method, "GET") == 0) {
        do_book_search(sock, request);
        return;
    }

    /* GET /api/stats —— 统计报表 */
    if (strcmp(path, "/api/stats") == 0 && strcmp(method, "GET") == 0) {
        do_stats(sock);
        return;
    }

    /* GET/POST /api/books/{id}/delist —— 下架图书 */
    if (strstr(path, "/api/books/") && strstr(path, "/delist")) {
        do_delist_from_url(sock, request, path);
        return;
    }

    /* GET /api/books/{id} —— 图书详情 */
    if (strncmp(path, "/api/books/", 11) == 0 && strlen(path) > 11 &&
        strcmp(method, "GET") == 0 && !strstr(path + 11, "/")) {
        do_book_detail(sock, path);
        return;
    }

    /* ===== 路由表匹配 ===== */
    for (int i = 0; i < g_route_count; i++) {
        if (strcmp(path, g_routes[i].path) == 0 &&
            strcmp(method, g_routes[i].method) == 0) {
            /* 提取 POST body */
            const char *body = (strcmp(method, "POST") == 0)
                             ? strstr(request, "\r\n\r\n")
                             : NULL;
            if (body) body += 4;

            char *json_res = g_routes[i].handler(body);
            send_json(sock, json_res ? json_res :
                      "{\"code\": 500, \"message\": \"内部错误\"}");
            free(json_res);
            return;
        }
    }

    /* 404 */
    send_json(sock, "{\"code\": 404, \"message\": \"接口不存在\"}");
}

/*==========================================================================
 * Ctrl+C 信号处理 —— 优雅退出
 *==========================================================================*/
static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_CLOSE_EVENT) {
        printf("\n[Server] 正在关闭服务器...\n");
        close_database();
        printf("[Server] 数据已保存，服务器已安全退出\n");
        ExitProcess(0);
    }
    return TRUE;
}

/*==========================================================================
 * 主函数
 *==========================================================================*/

int main(void) {
    /* 注册优雅退出处理 */
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup 失败!\n");
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET) {
        fprintf(stderr, "创建 socket 失败!\n");
        WSACleanup();
        return 1;
    }

    /* 允许端口快速复用 */
    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "端口 %d 绑定失败!\n", PORT);
        closesocket(server);
        WSACleanup();
        return 1;
    }

    listen(server, 10);

    /* 初始化数据库 */
    if (init_database() != STATUS_OK) {
        fprintf(stderr, "数据库初始化失败!\n");
        closesocket(server);
        WSACleanup();
        return 1;
    }

    /* 欢迎信息 */
    printf("\n");
    printf("  ╔══════════════════════════════════════════════════╗\n");
    printf("  ║  📚  校园二手书交易管理系统  v2.0                  ║\n");
    printf("  ╠══════════════════════════════════════════════════╣\n");
    printf("  ║  服务器已启动                                      ║\n");
    printf("  ║  地址: http://localhost:%d                        ║\n", PORT);
    printf("  ║  按 Ctrl+C 安全退出（自动保存数据）                 ║\n");
    printf("  ╚══════════════════════════════════════════════════╝\n");
    printf("\n");

    /* 主循环 */
    while (1) {
        SOCKET client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        char buffer[BUFFER_SIZE] = {0};
        int recv_len = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            handle_request(client, buffer);
        }

        closesocket(client);
    }

    close_database();
    closesocket(server);
    WSACleanup();
    return 0;
}