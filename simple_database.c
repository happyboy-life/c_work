#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include "database.h"
#include "data_persistence.h"

/*==========================================================================
 * 全局变量 —— 数据库句柄与动态链表缓存
 *==========================================================================*/
#define DATA_FILE      "books.dat"
#define USER_DATA_FILE "users.dat"

static sqlite3 *g_db = NULL;        /* 全局数据库句柄 */
static Book   *g_book_cache = NULL; /* 在售+历史图书全量链表缓存 */
static User   *g_user_cache = NULL; /* 用户全量链表缓存 */

// 安全释放 SQLite 错误信息
#define SAFE_FREE_ERR(e)  do { if (e) { sqlite3_free(e); (e) = NULL; } } while(0)

/*==========================================================================
 * 内联辅助函数 —— 减少重复代码
 *==========================================================================*/

/*
 * 从 JSON 字符串中提取指定键的值
 * 支持两种格式: "key":"value" 和 "key":number
 * 返回: out_buf 的指针（方便链式调用），未找到则返回空串
 */
static const char* json_extract(const char *json, const char *key,
                                char *out_buf, size_t buf_size) {
    char pattern[128];
    char *pos;
    size_t i;

    if (!json || !key || !out_buf || !buf_size) {
        if (out_buf) out_buf[0] = '\0';
        return out_buf;
    }

    // 先尝试带引号的字符串值: "key":"..."
    snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    pos = strstr((char*)json, pattern);
    if (pos) {
        pos += strlen(pattern);
        for (i = 0; *pos && *pos != '"' && i < buf_size - 1; i++)
            out_buf[i] = *pos++;
        out_buf[i] = '\0';
        return out_buf;
    }

    // 再尝试不带引号的数值: "key":number
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    pos = strstr((char*)json, pattern);
    if (pos) {
        pos += strlen(pattern);
        while (*pos == ' ' || *pos == ':') pos++;
        for (i = 0; *pos && *pos != ',' && *pos != '}' && *pos != ']'
             && i < buf_size - 1; pos++)
            if (*pos != ' ') out_buf[i++] = *pos;
        out_buf[i] = '\0';
        return out_buf;
    }

    out_buf[0] = '\0';
    return out_buf;
}

/*
 * 执行 SQL 语句（无返回值）
 * 成功返回 STATUS_OK(0)，失败返回 STATUS_DB_ERROR(-2)
 */
static int db_exec_simple(const char *sql) {
    char *err = NULL;
    int rc = sqlite3_exec(g_db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] SQL错误: %s\n  SQL: %s\n", err ? err : "unknown", sql);
        SAFE_FREE_ERR(err);
        return STATUS_DB_ERROR;
    }
    return STATUS_OK;
}

/*
 * 将单行结果集映射到 Book 结构体（减少重复的列读取逻辑）
 * 列顺序: id, name, author, isbn, publisher, category, condition,
 *          price, stock, status, user_id, seller_name, image_url, create_time
 */
static void stmt_to_book(sqlite3_stmt *stmt, Book *book) {
    book->id        = sqlite3_column_int(stmt, 0);
    strcpy(book->name,       (const char*)sqlite3_column_text(stmt, 1));
    strcpy(book->author,     (const char*)sqlite3_column_text(stmt, 2));
    strcpy(book->isbn,       sqlite3_column_text(stmt, 3)
                                ? (const char*)sqlite3_column_text(stmt, 3) : "");
    strcpy(book->publisher,  sqlite3_column_text(stmt, 4)
                                ? (const char*)sqlite3_column_text(stmt, 4) : "");
    strcpy(book->category,   (const char*)sqlite3_column_text(stmt, 5));
    strcpy(book->condition,  (const char*)sqlite3_column_text(stmt, 6));
    book->price     = (float)sqlite3_column_double(stmt, 7);
    book->stock     = sqlite3_column_int(stmt, 8);
    book->status    = sqlite3_column_int(stmt, 9);
    book->user_id   = sqlite3_column_int(stmt, 10);
    strcpy(book->seller_name, sqlite3_column_text(stmt, 11)
                                ? (const char*)sqlite3_column_text(stmt, 11) : "");
    strcpy(book->image_url,   sqlite3_column_text(stmt, 12)
                                ? (const char*)sqlite3_column_text(stmt, 12) : "");
    strcpy(book->create_time, sqlite3_column_text(stmt, 13)
                                ? (const char*)sqlite3_column_text(stmt, 13) : "");
}

/*
 * 从 SQL 查询构建 Book 链表（使用尾插法保持原始顺序）
 * 返回: 链表头指针（调用者负责用 FREE_LIST 宏释放）
 */
static Book* build_book_list(const char *sql, int *out_count) {
    sqlite3_stmt *stmt = NULL;
    Book *head = NULL, *tail = NULL, *node;
    int count = 0;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        node = (Book*)calloc(1, sizeof(Book));
        if (!node) break;
        stmt_to_book(stmt, node);

        /* 尾插法构建链表 */
        if (!head) head = tail = node;
        else { tail->next = node; tail = node; }
        count++;
    }
    sqlite3_finalize(stmt);

    if (out_count) *out_count = count;
    return head;
}

/*
 * 在全局链表缓存中按 ID 查找图书节点
 * 返回: 若找到返回节点指针，否则返回 NULL
 */
static Book* find_book_in_cache(int book_id) {
    Book *p = g_book_cache;
    while (p) {
        if (p->id == book_id) return p;
        p = p->next;
    }
    return NULL;
}

/*
 * 从数据库全量重建全局链表缓存
 * 应在: 初始化完成、增/删/改（影响缓存一致性）之后调用
 */
static void rebuild_book_cache(void) {
    /* 先释放旧缓存 */
    FREE_LIST(g_book_cache, Book);

    /* 从数据库全量加载所有图书记录 */
    int count = 0;
    g_book_cache = build_book_list(
        "SELECT id, name, author, isbn, publisher, category, condition, "
        "price, stock, status, user_id, seller_name, image_url, create_time "
        "FROM books ORDER BY id DESC;",
        &count);
    printf("[Cache] 链表缓存已重建，共 %d 条记录\n", count);
}

/*==========================================================================
 * 数据库生命周期
 *==========================================================================*/

/*
 * 修复 WAL 残留 —— 如果上次异常退出，WAL 文件可能导致数据库打开失败
 * 通过清理 -wal 和 -shm 文件来恢复数据库到一致状态
 */
static void wal_recovery(const char *db_path) {
    /* 先尝试通过 WAL checkpoint 恢复 */
    sqlite3 *db_test = NULL;
    int rc = sqlite3_open_v2(db_path, &db_test,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc == SQLITE_OK && db_test) {
        /* 强制 WAL checkpoint 将所有日志写入主文件 */
        sqlite3_exec(db_test, "PRAGMA wal_checkpoint(TRUNCATE);", NULL, NULL, NULL);
        sqlite3_close(db_test);
        printf("[DB] WAL 恢复完成 (checkpoint)\n");
    }

    /* 删除残留的 WAL/SHM 文件（如果 checkpoint 后仍然存在） */
    remove("bookstore.db-wal");
    remove("bookstore.db-shm");
    printf("[DB] WAL 残留文件已清理\n");
}

int init_database(void) {
    /* Step 1: 在打开数据库之前先清理 WAL 残留 */
    wal_recovery("bookstore.db");

    /* Step 2: 打开数据库（多次重试） */
    int retry = 0;
    int rc;
    while (retry < 3) {
        rc = sqlite3_open("bookstore.db", &g_db);
        if (rc == SQLITE_OK) break;
        fprintf(stderr, "[DB] 第 %d 次打开失败 (rc=%d): %s\n",
                retry + 1, rc, g_db ? sqlite3_errmsg(g_db) : "N/A");
        if (g_db) { sqlite3_close(g_db); g_db = NULL; }
        /* 再次清理残留并重试 */
        wal_recovery("bookstore.db");
        retry++;
    }

    if (rc != SQLITE_OK) {
        fprintf(stderr, "[DB] 3次重试均失败，将依赖 books.dat 文件运行\n");
        /* 数据库完全不可用，尝试从数据文件加载 */
        int fcount = 0;
        int fret = load_books_from_file(&g_book_cache, &fcount, DATA_FILE);
        if (fret == STATUS_OK) {
            printf("[DB] 已从 %s 恢复 %d 条记录到链表缓存 (SQLite不可用)\n",
                   DATA_FILE, fcount);
            return STATUS_OK;
        }
        fprintf(stderr, "[DB] 数据文件也无法加载，以空缓存启动\n");
        return STATUS_OK;  /* 以空缓存运行，不阻断服务器启动 */
    }

    /* Step 3: 启用 WAL 模式与外键 */
    sqlite3_exec(g_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(g_db, "PRAGMA foreign_keys=ON;", NULL, NULL, NULL);

    /* Step 4: 创建表结构 */
    const char *schemas[] = {
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE NOT NULL, "
        "password TEXT NOT NULL, "
        "is_profile_complete INTEGER DEFAULT 0);",

        "CREATE TABLE IF NOT EXISTS books ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "author TEXT, "
        "isbn TEXT DEFAULT '', "
        "publisher TEXT DEFAULT '', "
        "category TEXT, "
        "condition TEXT, "
        "price REAL, "
        "stock INTEGER DEFAULT 1, "
        "status INTEGER DEFAULT 0, "
        "user_id INTEGER, "
        "seller_name TEXT DEFAULT '', "
        "image_url TEXT, "
        "create_time TEXT DEFAULT (datetime('now','localtime')));",

        "CREATE TABLE IF NOT EXISTS collects ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER, "
        "book_id INTEGER);",

        "CREATE TABLE IF NOT EXISTS user_profiles ("
        "user_id INTEGER PRIMARY KEY, "
        "name TEXT, "
        "class_name TEXT, "
        "student_id TEXT, "
        "avatar TEXT);",

        "CREATE TABLE IF NOT EXISTS purchases ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER, "
        "book_id INTEGER, "
        "buyer_name TEXT DEFAULT '', "
        "transaction_time TEXT DEFAULT (datetime('now','localtime')));"
    };

    size_t n = sizeof(schemas) / sizeof(schemas[0]);
    for (size_t i = 0; i < n; i++) {
        if (db_exec_simple(schemas[i]) != STATUS_OK) {
            fprintf(stderr, "[DB] 建表失败，尝试从数据文件恢复\n");
            /* 建表失败但数据库可能部分可用，尝试加载数据文件 */
            int fcount = 0;
            load_books_from_file(&g_book_cache, &fcount, DATA_FILE);
            printf("[DB] 以降级模式启动 (缓存=%d条)\n", fcount);
            return STATUS_OK;
        }
    }

    /* Step 5: 初始化图书链表缓存 —— 优先从 SQLite，失败则回退到 books.dat */
    rebuild_book_cache();

    /* 如果数据库为空但 books.dat 有数据，同步恢复 */
    if (!g_book_cache) {
        int fcount = 0;
        int fret = load_books_from_file(&g_book_cache, &fcount, DATA_FILE);
        if (fret == STATUS_OK) {
            printf("[DB] 数据库为空，已从 %s 恢复 %d 条记录\n",
                   DATA_FILE, fcount);
            /* 将数据文件中的记录写回 SQLite */
            Book *p = g_book_cache;
            while (p) {
                char sql[1536];
                snprintf(sql, sizeof(sql),
                    "INSERT INTO books (id, name, author, isbn, publisher, "
                    "category, condition, price, stock, status, user_id, "
                    "seller_name, image_url, create_time) "
                    "VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', "
                    "%.2f, %d, %d, %d, '%s', '%s', '%s');",
                    p->id, p->name, p->author, p->isbn, p->publisher,
                    p->category, p->condition, p->price, p->stock,
                    p->status, p->user_id, p->seller_name,
                    p->image_url, p->create_time);
                db_exec_simple(sql);
                p = p->next;
            }
            printf("[DB] 数据文件记录已同步回 SQLite\n");
        }
    } else {
        /* SQLite 有数据，立即同步到数据文件 */
        save_books_to_file(g_book_cache, DATA_FILE);
    }

    /* Step 6: 初始化用户链表缓存 */
    {
        /* 尝试从 users.dat 加载 */
        int ucount = 0;
        int uret = load_users_from_file(&g_user_cache, &ucount, USER_DATA_FILE);
        if (uret == STATUS_OK) {
            printf("[DB] 已从 %s 加载 %d 条用户记录\n", USER_DATA_FILE, ucount);
        } else if (uret == STATUS_NOT_FOUND) {
            printf("[DB] %s 不存在，从 SQLite 重建\n", USER_DATA_FILE);
            /* 从 SQLite 加载所有用户 */
            sqlite3_stmt *ustmt = NULL;
            if (sqlite3_prepare_v2(g_db,
                    "SELECT id, username, password, is_profile_complete "
                    "FROM users ORDER BY id;",
                    -1, &ustmt, NULL) == SQLITE_OK) {
                User *utail = NULL;
                int loaded = 0;
                while (sqlite3_step(ustmt) == SQLITE_ROW) {
                    User *u = (User*)calloc(1, sizeof(User));
                    if (!u) break;
                    u->id = sqlite3_column_int(ustmt, 0);
                    strcpy(u->username, (const char*)sqlite3_column_text(ustmt, 1));
                    strcpy(u->password, (const char*)sqlite3_column_text(ustmt, 2));
                    u->is_profile_complete = sqlite3_column_int(ustmt, 3);
                    u->next = NULL;
                    if (!g_user_cache) g_user_cache = utail = u;
                    else { utail->next = u; utail = u; }
                    loaded++;
                }
                sqlite3_finalize(ustmt);
                if (loaded > 0) {
                    save_users_to_file(g_user_cache, USER_DATA_FILE);
                    printf("[DB] 从 SQLite 加载了 %d 条用户并保存到 %s\n",
                           loaded, USER_DATA_FILE);
                }
            }
        }
    }

    puts("[DB] 数据库初始化完成，链表缓存与数据文件已同步");
    return STATUS_OK;
}

/* 将链表缓存同步到数据文件（供外部调用） */
int sync_book_cache_to_file(void) {
    return save_books_to_file(g_book_cache, DATA_FILE);
}

/* 将用户缓存同步到数据文件（供外部调用） */
int sync_user_cache_to_file(void) {
    return save_users_to_file(g_user_cache, USER_DATA_FILE);
}

void close_database(void) {
    /* 退出前保存数据到文件 */
    save_books_to_file(g_book_cache, DATA_FILE);
    save_users_to_file(g_user_cache, USER_DATA_FILE);
    /* 强制 WAL checkpoint 确保数据写入主文件 */
    if (g_db) sqlite3_exec(g_db, "PRAGMA wal_checkpoint(TRUNCATE);", NULL, NULL, NULL);
    FREE_LIST(g_book_cache, Book);  /* 释放图书链表缓存 */
    FREE_LIST(g_user_cache, User);  /* 释放用户链表缓存 */
    if (g_db) { sqlite3_close(g_db); g_db = NULL; }
}

/*==========================================================================
 * 用户操作
 *==========================================================================*/

int register_user(const char *json_data) {
    char username[64] = {0}, password[128] = {0};
    json_extract(json_data, "username", username, sizeof(username));
    json_extract(json_data, "password", password, sizeof(password));

    if (!username[0] || !password[0]) return STATUS_INVALID_PARAM;

    /* 检查是否已存在 */
    char sql[256];
    sqlite3_stmt *stmt = NULL;
    snprintf(sql, sizeof(sql),
             "SELECT id FROM users WHERE username='%s';", username);
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return STATUS_EXISTS;
        }
        sqlite3_finalize(stmt);
    }

    snprintf(sql, sizeof(sql),
             "INSERT INTO users (username, password) VALUES ('%s', '%s');",
             username, password);
    return db_exec_simple(sql);
}

int login_user(const char *json_data, User **out_user) {
    char username[64] = {0}, password[128] = {0};
    json_extract(json_data, "username", username, sizeof(username));
    json_extract(json_data, "password", password, sizeof(password));

    if (!username[0] || !password[0]) return STATUS_INVALID_PARAM;

    /* Step 1: 尝试查找已有用户 */
    char sql[256];
    sqlite3_stmt *stmt = NULL;
    snprintf(sql, sizeof(sql),
             "SELECT id, username, is_profile_complete "
             "FROM users WHERE username='%s' AND password='%s';",
             username, password);

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            User *u = (User*)calloc(1, sizeof(User));
            if (!u) { sqlite3_finalize(stmt); return STATUS_DB_ERROR; }
            u->id                  = sqlite3_column_int(stmt, 0);
            strcpy(u->username,    (const char*)sqlite3_column_text(stmt, 1));
            u->is_profile_complete = sqlite3_column_int(stmt, 2);
            sqlite3_finalize(stmt);
            *out_user = u;
            return STATUS_OK;  /* 已有用户，验证密码通过 */
        }
        sqlite3_finalize(stmt);
    }

    /* Step 2: 检查用户名是否被占用（密码不匹配的情况） */
    snprintf(sql, sizeof(sql),
             "SELECT id FROM users WHERE username='%s';", username);
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return STATUS_NOT_FOUND;  /* 用户名存在但密码错误 → 返回登录失败 */
        }
        sqlite3_finalize(stmt);
    }

    /* Step 3: 用户名不存在 → 自动创建新账号（登录即注册） */
    snprintf(sql, sizeof(sql),
             "INSERT INTO users (username, password) VALUES ('%s', '%s');",
             username, password);
    if (db_exec_simple(sql) != STATUS_OK) return STATUS_DB_ERROR;

    int new_id = (int)sqlite3_last_insert_rowid(g_db);

    /* 将新用户插入链表缓存 */
    {
        User *u = (User*)calloc(1, sizeof(User));
        if (!u) return STATUS_DB_ERROR;
        u->id = new_id;
        strcpy(u->username, username);
        strcpy(u->password, password);
        u->is_profile_complete = 0;
        u->next = g_user_cache;
        g_user_cache = u;
    }

    /* 自动同步用户数据文件 */
    save_users_to_file(g_user_cache, USER_DATA_FILE);
    printf("[User] 新用户自动注册: id=%d, username=%s\n", new_id, username);

    /* 返回新创建的用户信息 */
    User *new_user = (User*)calloc(1, sizeof(User));
    if (!new_user) return STATUS_DB_ERROR;
    new_user->id = new_id;
    strcpy(new_user->username, username);
    new_user->is_profile_complete = 0;
    *out_user = new_user;

    return STATUS_CREATED;  /* 表示是新创建的账号 */
}

int get_user_profile(int user_id, UserProfile **out_profile) {
    char sql[256];
    sqlite3_stmt *stmt = NULL;
    snprintf(sql, sizeof(sql),
             "SELECT user_id, name, class_name, student_id, avatar "
             "FROM user_profiles WHERE user_id=%d;", user_id);

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return STATUS_DB_ERROR;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        UserProfile *p = (UserProfile*)calloc(1, sizeof(UserProfile));
        if (!p) { sqlite3_finalize(stmt); return STATUS_DB_ERROR; }
        p->user_id = sqlite3_column_int(stmt, 0);
        strcpy(p->name,       (const char*)sqlite3_column_text(stmt, 1));
        strcpy(p->class_name, (const char*)sqlite3_column_text(stmt, 2));
        strcpy(p->student_id, (const char*)sqlite3_column_text(stmt, 3));
        strcpy(p->avatar,     (const char*)sqlite3_column_text(stmt, 4));
        sqlite3_finalize(stmt);
        *out_profile = p;
        return STATUS_OK;
    }

    sqlite3_finalize(stmt);
    return STATUS_NOT_FOUND;
}

int update_user_profile(const char *json_data) {
    char uid_str[16] = {0}, name[64] = {0}, class_name[64] = {0};
    char student_id[32] = {0}, avatar[256] = {0};

    json_extract(json_data, "userId",    uid_str,     sizeof(uid_str));
    json_extract(json_data, "name",      name,        sizeof(name));
    json_extract(json_data, "class",     class_name,  sizeof(class_name));
    json_extract(json_data, "studentId", student_id,  sizeof(student_id));
    json_extract(json_data, "avatar",    avatar,      sizeof(avatar));

    int user_id = atoi(uid_str);
    if (user_id <= 0) return STATUS_INVALID_PARAM;

    char sql[512];
    snprintf(sql, sizeof(sql),
             "INSERT OR REPLACE INTO user_profiles "
             "(user_id, name, class_name, student_id, avatar) "
             "VALUES (%d, '%s', '%s', '%s', '%s');",
             user_id, name, class_name, student_id, avatar);

    if (db_exec_simple(sql) != STATUS_OK) return STATUS_DB_ERROR;

    /* 标记资料已完善 */
    snprintf(sql, sizeof(sql),
             "UPDATE users SET is_profile_complete=1 WHERE id=%d;", user_id);
    db_exec_simple(sql);

    return STATUS_OK;
}

/*==========================================================================
 * 图书操作 —— 涉及链表缓存同步
 *==========================================================================*/

int add_book(const char *json_data) {
    char name[128] = {0}, author[128] = {0}, isbn[32] = {0};
    char publisher[128] = {0}, category[64] = {0}, condition[32] = {0};
    char price_str[32] = {0}, stock_str[16] = {0};
    char uid_str[16] = {0}, image[256] = {0}, seller_name[64] = {0};

    json_extract(json_data, "name",        name,         sizeof(name));
    json_extract(json_data, "author",      author,       sizeof(author));
    json_extract(json_data, "isbn",        isbn,         sizeof(isbn));
    json_extract(json_data, "publisher",   publisher,    sizeof(publisher));
    json_extract(json_data, "category",    category,     sizeof(category));
    json_extract(json_data, "condition",   condition,    sizeof(condition));
    json_extract(json_data, "price",       price_str,    sizeof(price_str));
    json_extract(json_data, "stock",       stock_str,    sizeof(stock_str));
    json_extract(json_data, "userId",      uid_str,      sizeof(uid_str));
    json_extract(json_data, "image",       image,        sizeof(image));
    json_extract(json_data, "sellerName",  seller_name,  sizeof(seller_name));

    if (!name[0]) return STATUS_INVALID_PARAM;

    float price  = (float)atof(price_str);
    int   stock  = atoi(stock_str);
    int   user_id = atoi(uid_str);

    /* 如果没有传入 sellerName，则从 user_profiles 中查找 */
    if (!seller_name[0] && user_id > 0) {
        char sql[256];
        sqlite3_stmt *stmt = NULL;
        snprintf(sql, sizeof(sql),
                 "SELECT name FROM user_profiles WHERE user_id=%d;", user_id);
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                strcpy(seller_name,
                       (const char*)sqlite3_column_text(stmt, 0));
            }
            sqlite3_finalize(stmt);
        }
    }

    char sql[768];
    snprintf(sql, sizeof(sql),
             "INSERT INTO books "
             "(name, author, isbn, publisher, category, condition, "
             "price, stock, status, user_id, seller_name, image_url) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', "
             "%.2f, %d, %d, %d, '%s', '%s');",
             name, author, isbn, publisher, category, condition,
             price, stock, BOOK_STATUS_ON_SALE, user_id, seller_name, image);

    if (db_exec_simple(sql) != STATUS_OK) return STATUS_DB_ERROR;

    int new_id = (int)sqlite3_last_insert_rowid(g_db);

    /* 将新图书插入链表缓存头部 */
    {
        Book *node = (Book*)calloc(1, sizeof(Book));
        if (node) {
            node->id     = new_id;
            strcpy(node->name,      name);
            strcpy(node->author,    author);
            strcpy(node->isbn,      isbn);
            strcpy(node->publisher, publisher);
            strcpy(node->category,  category);
            strcpy(node->condition, condition);
            node->price   = price;
            node->stock   = stock;
            node->status  = BOOK_STATUS_ON_SALE;
            node->user_id = user_id;
            strcpy(node->seller_name, seller_name);
            strcpy(node->image_url,   image);
            /* create_time 由数据库默认值生成，此处从数据库查询填入 */
            {
                char tsql[256];
                sqlite3_stmt *tstmt = NULL;
                snprintf(tsql, sizeof(tsql),
                         "SELECT create_time FROM books WHERE id=%d;", new_id);
                if (sqlite3_prepare_v2(g_db, tsql, -1, &tstmt, NULL) == SQLITE_OK) {
                    if (sqlite3_step(tstmt) == SQLITE_ROW)
                        strcpy(node->create_time,
                               (const char*)sqlite3_column_text(tstmt, 0));
                    sqlite3_finalize(tstmt);
                }
            }
            /* 插入链表头部 */
            node->next = g_book_cache;
            g_book_cache = node;
        }
    }

    /* 自动同步数据文件 */
    save_books_to_file(g_book_cache, DATA_FILE);
    return new_id;  /* 成功时返回新书 ID */
}

int get_all_books(Book **out_books, int *out_count) {
    /* 直接从链表缓存中筛选在售图书，避免数据库查询 */
    Book *result_head = NULL, *result_tail = NULL, *node = NULL;
    Book *p = g_book_cache;
    int count = 0;

    while (p) {
        if (p->status == BOOK_STATUS_ON_SALE && p->stock > 0) {
            node = (Book*)calloc(1, sizeof(Book));
            if (!node) break;
            memcpy(node, p, sizeof(Book));
            node->next = NULL;
            if (!result_head) result_head = result_tail = node;
            else { result_tail->next = node; result_tail = node; }
            count++;
        }
        p = p->next;
    }

    *out_books  = result_head;
    *out_count  = count;
    return STATUS_OK;
}

int get_book_by_id(int book_id, Book **out_book) {
    /* 优先从链表缓存查找 */
    Book *cached = find_book_in_cache(book_id);
    if (cached) {
        Book *b = (Book*)calloc(1, sizeof(Book));
        if (!b) return STATUS_DB_ERROR;
        memcpy(b, cached, sizeof(Book));
        *out_book = b;
        return STATUS_OK;
    }

    /* 缓存未命中，回退数据库查询 */
    char sql[512];
    sqlite3_stmt *stmt = NULL;
    snprintf(sql, sizeof(sql),
             "SELECT id, name, author, isbn, publisher, category, condition, "
             "price, stock, status, user_id, seller_name, image_url, create_time "
             "FROM books WHERE id=%d;", book_id);

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return STATUS_DB_ERROR;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Book *b = (Book*)calloc(1, sizeof(Book));
        if (!b) { sqlite3_finalize(stmt); return STATUS_DB_ERROR; }
        stmt_to_book(stmt, b);
        sqlite3_finalize(stmt);
        *out_book = b;
        return STATUS_OK;
    }

    sqlite3_finalize(stmt);
    return STATUS_NOT_FOUND;
}

/*==========================================================================
 * 图书搜索（多条件查询）
 *==========================================================================*/

int search_books(const char *keyword, const char *author,
                 const char *isbn, float min_price, float max_price,
                 int seller_id, const char *seller_student_id,
                 int status, const char *category,
                 Book **out_books, int *out_count) {
    /* 
     * 从链表缓存中按条件过滤
     * 无任何条件时返回所有记录（包括在售、已售、下架）
     */
    int has_condition = (keyword && keyword[0])
                     || (author && author[0])
                     || (isbn && isbn[0])
                     || (min_price > 0)
                     || (max_price > 0)
                     || (seller_id > 0)
                     || (seller_student_id && seller_student_id[0])
                     || (status >= 0)
                     || (category && category[0]);

    /* 如果按学号搜索，先查询学号对应的 user_id 集合 */
    int seller_user_id_from_student = 0;
    if (seller_student_id && seller_student_id[0]) {
        char sql[256];
        sqlite3_stmt *stmt = NULL;
        snprintf(sql, sizeof(sql),
                 "SELECT user_id FROM user_profiles WHERE student_id='%s';",
                 seller_student_id);
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                seller_user_id_from_student = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    }

    Book *result_head = NULL, *result_tail = NULL;
    int count = 0;

    Book *p = g_book_cache;
    while (p) {
        int match = 1;

        /* 默认无筛选时返回所有 */
        if (has_condition) {
            if (keyword && keyword[0]) {
                if (!strstr(p->name, keyword)
                    && !strstr(p->author, keyword)
                    && !strstr(p->isbn, keyword))
                    match = 0;
            }
            if (author && author[0] && !strstr(p->author, author)) match = 0;
            if (isbn && isbn[0] && !strstr(p->isbn, isbn)) match = 0;
            if (min_price > 0 && p->price < min_price) match = 0;
            if (max_price > 0 && p->price > max_price) match = 0;
            if (seller_id > 0 && p->user_id != seller_id) match = 0;
            if (seller_student_id && seller_student_id[0]
                && p->user_id != seller_user_id_from_student) match = 0;
            if (status >= 0 && p->status != status) match = 0;
            if (category && category[0] && strcmp(p->category, category) != 0)
                match = 0;
        }

        if (match) {
            Book *node = (Book*)calloc(1, sizeof(Book));
            if (!node) break;
            memcpy(node, p, sizeof(Book));
            node->next = NULL;
            if (!result_head) result_head = result_tail = node;
            else { result_tail->next = node; result_tail = node; }
            count++;
        }

        p = p->next;
    }

    *out_books  = result_head;
    *out_count  = count;
    return STATUS_OK;
}

/*==========================================================================
 * 收藏操作
 *==========================================================================*/

int add_collect(const char *json_data) {
    char uid_str[16] = {0}, bid_str[16] = {0};
    json_extract(json_data, "userId", uid_str, sizeof(uid_str));
    json_extract(json_data, "bookId", bid_str, sizeof(bid_str));

    int user_id = atoi(uid_str);
    int book_id = atoi(bid_str);
    if (user_id <= 0 || book_id <= 0) return STATUS_INVALID_PARAM;

    /* 去重检查 */
    char sql[128];
    sqlite3_stmt *stmt = NULL;
    snprintf(sql, sizeof(sql),
             "SELECT id FROM collects WHERE user_id=%d AND book_id=%d;",
             user_id, book_id);
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return STATUS_EXISTS;
        }
        sqlite3_finalize(stmt);
    }

    snprintf(sql, sizeof(sql),
             "INSERT INTO collects (user_id, book_id) VALUES (%d, %d);",
             user_id, book_id);
    return db_exec_simple(sql);
}

int get_collect_books(int user_id, Book **out_books, int *out_count) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT b.id, b.name, b.author, b.isbn, b.publisher, "
             "b.category, b.condition, b.price, b.stock, b.status, "
             "b.user_id, b.seller_name, b.image_url, b.create_time "
             "FROM books b INNER JOIN collects c ON b.id=c.book_id "
             "WHERE c.user_id=%d ORDER BY c.id DESC;", user_id);
    *out_books = build_book_list(sql, out_count);
    return STATUS_OK;
}

/*==========================================================================
 * 发布与购买
 *==========================================================================*/

int get_published_books(int user_id, Book **out_books, int *out_count) {
    /* 从链表缓存中按 seller_id 筛选 */
    Book *result_head = NULL, *result_tail = NULL;
    int count = 0;
    Book *p = g_book_cache;

    while (p) {
        if (p->user_id == user_id) {
            Book *node = (Book*)calloc(1, sizeof(Book));
            if (!node) break;
            memcpy(node, p, sizeof(Book));
            node->next = NULL;
            if (!result_head) result_head = result_tail = node;
            else { result_tail->next = node; result_tail = node; }
            count++;
        }
        p = p->next;
    }

    *out_books = result_head;
    *out_count = count;
    return STATUS_OK;
}

int get_purchased_books(int user_id, Book **out_books, int *out_count) {
    /* 已购记录涉及 JOIN 查询，从数据库获取 */
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "SELECT b.id, b.name, b.author, b.isbn, b.publisher, "
             "b.category, b.condition, b.price, b.stock, b.status, "
             "b.user_id, b.seller_name, b.image_url, b.create_time "
             "FROM books b INNER JOIN purchases p ON b.id=p.book_id "
             "WHERE p.user_id=%d ORDER BY p.id DESC;", user_id);
    *out_books = build_book_list(sql, out_count);
    return STATUS_OK;
}

int purchase_book(const char *json_data) {
    char uid_str[16] = {0}, bid_str[16] = {0}, buyer_name[64] = {0};
    json_extract(json_data, "userId", uid_str, sizeof(uid_str));
    json_extract(json_data, "bookId", bid_str, sizeof(bid_str));
    json_extract(json_data, "buyerName", buyer_name, sizeof(buyer_name));

    int user_id = atoi(uid_str);
    int book_id = atoi(bid_str);
    if (user_id <= 0 || book_id <= 0) return STATUS_INVALID_PARAM;

    /* 如果没有传入 buyerName，则从 user_profiles 中查找 */
    if (!buyer_name[0]) {
        char sql[256];
        sqlite3_stmt *stmt = NULL;
        snprintf(sql, sizeof(sql),
                 "SELECT name FROM user_profiles WHERE user_id=%d;", user_id);
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                strcpy(buyer_name,
                       (const char*)sqlite3_column_text(stmt, 0));
            }
            sqlite3_finalize(stmt);
        }
    }

    /* 查库存和状态 —— 优先从缓存读取 */
    Book *cached = find_book_in_cache(book_id);
    if (!cached) return STATUS_NOT_FOUND;

    int stock  = cached->stock;
    int status = cached->status;

    if (stock <= 0) return STATUS_OUT_OF_STOCK;
    if (status != BOOK_STATUS_ON_SALE) return STATUS_NOT_FOUND;

    /* 减库存，标记状态 */
    int new_stock = stock - 1;
    char sql[256];
    snprintf(sql, sizeof(sql),
             "UPDATE books SET stock = %d, status = %s "
             "WHERE id=%d AND stock > 0;",
             new_stock,
             new_stock <= 0 ? "1" : "0",
             book_id);
    if (db_exec_simple(sql) != STATUS_OK) return STATUS_DB_ERROR;

    /* 记购买记录 */
    snprintf(sql, sizeof(sql),
             "INSERT INTO purchases (user_id, book_id, buyer_name) "
             "VALUES (%d, %d, '%s');",
             user_id, book_id, buyer_name);
    int ret = db_exec_simple(sql);

    /* 同步更新链表缓存 */
    if (ret == STATUS_OK) {
        cached->stock  = new_stock;
        cached->status = new_stock <= 0 ? BOOK_STATUS_SOLD : BOOK_STATUS_ON_SALE;
    }

    /* 自动同步数据文件 */
    if (ret == STATUS_OK) save_books_to_file(g_book_cache, DATA_FILE);
    return ret;
}

/*==========================================================================
 * 下架图书 —— 同步更新链表缓存
 *==========================================================================*/

int delist_book(int book_id, int user_id, const char *student_id) {
    /* 先从缓存验证 */
    Book *cached = find_book_in_cache(book_id);
    int book_owner_id = 0;
    int status = -1;

    if (!cached) {
        /* 缓存未找到，从数据库查询 */
        char sql[256];
        sqlite3_stmt *stmt = NULL;
        snprintf(sql, sizeof(sql),
                 "SELECT status, user_id FROM books WHERE id=%d;", book_id);
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
            return STATUS_DB_ERROR;
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return STATUS_NOT_FOUND;
        }
        status       = sqlite3_column_int(stmt, 0);
        book_owner_id = sqlite3_column_int(stmt, 1);
        sqlite3_finalize(stmt);

        if (book_owner_id != user_id) return STATUS_INVALID_PARAM;
        if (status != BOOK_STATUS_ON_SALE) return STATUS_NOT_FOUND;
    } else {
        book_owner_id = cached->user_id;
        /* 验证卖家身份 */
        if (book_owner_id != user_id) return STATUS_INVALID_PARAM;
        /* 只能下架"在售"状态的图书 */
        if (cached->status != BOOK_STATUS_ON_SALE) return STATUS_NOT_FOUND;
    }

    /* ===========================================================
     * 核对发布者学号
     * 从 user_profiles 中查询发布者的 student_id 进行比对
     * ===========================================================*/
    if (student_id && student_id[0]) {
        char sql[256];
        sqlite3_stmt *stmt = NULL;
        snprintf(sql, sizeof(sql),
                 "SELECT student_id FROM user_profiles WHERE user_id=%d;",
                 book_owner_id);
        if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char *db_student_id =
                    (const char*)sqlite3_column_text(stmt, 0);
                if (!db_student_id || strcmp(db_student_id, student_id) != 0) {
                    sqlite3_finalize(stmt);
                    fprintf(stderr,
                        "[DB] 学号验证失败: 输入=%s, 数据库=%s\n",
                        student_id, db_student_id ? db_student_id : "(null)");
                    return STATUS_INVALID_PARAM;  /* 学号不匹配 */
                }
            } else {
                /* 发布者没有填写学号信息，无法核对 */
                sqlite3_finalize(stmt);
                fprintf(stderr,
                    "[DB] 学号验证失败: 发布者(user_id=%d)未填写学号信息\n",
                    book_owner_id);
                return STATUS_INVALID_PARAM;
            }
            sqlite3_finalize(stmt);
        } else {
            return STATUS_DB_ERROR;
        }
    }

    char upsql[256];
    snprintf(upsql, sizeof(upsql),
             "UPDATE books SET status = %d WHERE id=%d;",
             BOOK_STATUS_DELISTED, book_id);
    int ret = db_exec_simple(upsql);

    /* 同步更新缓存 */
    if (ret == STATUS_OK && cached)
        cached->status = BOOK_STATUS_DELISTED;

    /* 自动同步数据文件 */
    if (ret == STATUS_OK) save_books_to_file(g_book_cache, DATA_FILE);
    return ret;
}

/*==========================================================================
 * 统计报表
 *==========================================================================*/

int get_stats(Stats **out_stats) {
    Stats *s = (Stats*)calloc(1, sizeof(Stats));
    if (!s) return STATUS_DB_ERROR;

    sqlite3_stmt *stmt = NULL;

    /* 总上架数量 */
    if (sqlite3_prepare_v2(g_db,
            "SELECT COUNT(*) FROM books;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            s->total_listed = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }

    /* 已售数量 */
    if (sqlite3_prepare_v2(g_db,
            "SELECT COUNT(*) FROM books WHERE status = 1;",
            -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            s->total_sold = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }

    /* 下架数量 */
    if (sqlite3_prepare_v2(g_db,
            "SELECT COUNT(*) FROM books WHERE status = 2;",
            -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            s->total_delisted = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }

    /* 总交易金额 */
    if (sqlite3_prepare_v2(g_db,
            "SELECT COALESCE(SUM(price), 0) FROM books WHERE status = 1;",
            -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            s->total_amount = (float)sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
    }

    /* 按分类统计成交量 */
    if (sqlite3_prepare_v2(g_db,
            "SELECT category, COUNT(*) as cnt FROM books "
            "WHERE status = 1 GROUP BY category ORDER BY cnt DESC;",
            -1, &stmt, NULL) == SQLITE_OK) {
        int i = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW && i < 32) {
            strcpy(s->categories[i],
                   (const char*)sqlite3_column_text(stmt, 0));
            s->category_sold[i] = sqlite3_column_int(stmt, 1);
            i++;
        }
        s->category_count = i;
        sqlite3_finalize(stmt);
    }

    *out_stats = s;
    return STATUS_OK;
}