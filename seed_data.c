/*
 * 种子数据初始化程序
 *
 * 功能：
 *   - 创建所有必要的数据库表结构
 *   - 插入模拟卖家用户及用户资料
 *   - 插入模拟图书数据
 *
 * 用法：
 *   1. 编译：gcc -o seed_data.exe seed_data.c -lsqlite3 -lws2_32
 *   2. 运行：seed_data.exe
 *
 * 依赖：
 *   - SQLite3 库 (libsqlite3)
 *   - bookstore.db 数据库文件
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

/*==========================================================================
 * 常量定义
 *==========================================================================*/

#define DB_NAME     "bookstore.db"
#define SEED_COUNT  8          // 模拟图书数量
#define USER_COUNT  8          // 模拟用户数量

/*==========================================================================
 * 辅助函数
 *==========================================================================*/

/* 执行单条 SQL 语句，失败时打印错误并返回 -1 */
static int exec_sql(sqlite3 *db, const char *sql) {
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[错误] SQL执行失败: %s\nSQL: %.120s\n", err_msg, sql);
        sqlite3_free(err_msg);
        return -1;
    }
    return 0;
}

/*==========================================================================
 * 步骤 1：创建表结构
 *==========================================================================*/

static int create_tables(sqlite3 *db) {
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

    printf("[1/4] 创建表结构...\n");

    size_t n = sizeof(schemas) / sizeof(schemas[0]);
    for (size_t i = 0; i < n; i++) {
        if (exec_sql(db, schemas[i]) != 0) {
            fprintf(stderr, "[错误] 创建第 %zu 个表失败\n", i + 1);
            return -1;
        }
    }

    printf("  -> 5 张表创建/确认完毕\n");
    return 0;
}

/*==========================================================================
 * 步骤 2：插入模拟用户
 *==========================================================================*/

static int insert_users(sqlite3 *db) {
    const char *sql = 
        "INSERT OR IGNORE INTO users (id, username, password, is_profile_complete) VALUES "
        "(1, 'bookworm_lee',     '123456', 1),"
        "(2, 'programmer_wang',  '123456', 1),"
        "(3, 'history_fan',      '123456', 1),"
        "(4, 'econ_master',      '123456', 1),"
        "(5, 'exam_warrior',     '123456', 1),"
        "(6, 'art_youth',        '123456', 1),"
        "(7, 'science_geek',     '123456', 1),"
        "(8, 'philosophy_lover', '123456', 1);";

    printf("[2/4] 插入模拟用户...\n");

    if (exec_sql(db, sql) != 0) {
        return -1;
    }

    printf("  -> %d 位用户已创建\n", USER_COUNT);
    return 0;
}

/*==========================================================================
 * 步骤 3：插入用户资料
 *==========================================================================*/

static int insert_profiles(sqlite3 *db) {
    const char *sql = 
        "INSERT OR IGNORE INTO user_profiles (user_id, name, class_name, student_id, avatar) VALUES "
        "(1, '书虫小李',     '中文系2022级',   '2022001', ''),"
        "(2, '程序员小王',   '计算机系2021级', '2021002', ''),"
        "(3, '历史爱好者',   '历史系2022级',   '2022003', ''),"
        "(4, '经济学霸',     '经济学院2022级', '2022004', ''),"
        "(5, '考研党',       '数学系2020级',   '2020005', ''),"
        "(6, '文艺青年',     '文学院2022级',   '2022006', ''),"
        "(7, '科学达人',     '物理系2023级',   '2023007', ''),"
        "(8, '哲学思考者',   '哲学系2021级',   '2021008', '');";

    printf("[3/4] 插入用户资料...\n");

    if (exec_sql(db, sql) != 0) {
        return -1;
    }

    printf("  -> %d 份用户资料已创建\n", USER_COUNT);
    return 0;
}

/*==========================================================================
 * 步骤 4：插入模拟图书
 *==========================================================================*/

typedef struct {
    int    id;
    char   name[128];
    char   author[128];
    char   isbn[32];
    char   publisher[128];
    char   category[64];
    char   condition[32];
    float  price;
    int    stock;
    int    user_id;
    char   seller_name[64];
    char   image_url[256];
    char   create_time[32];
} SeedBook;

static int insert_books(sqlite3 *db) {
    const SeedBook books[] = {
        {1, "百年孤独",                   "加西亚·马尔克斯",   "9787544253994",
         "南海出版公司", "文学小说", "八成新",   25.00f, 3, 1,
         "书虫小李",   "https://picsum.photos/200/280?random=1",
         "2025-09-01 10:00:00"},

        {2, "JavaScript高级程序设计",      "马特·弗里斯比",     "9787115546081",
         "人民邮电出版社", "科学技术", "九成新",   55.00f, 2, 2,
         "程序员小王", "https://picsum.photos/200/280?random=2",
         "2025-08-15 14:30:00"},

        {3, "人类简史",                   "尤瓦尔·赫拉利",     "9787508647357",
         "中信出版社", "人文社科", "七成新",   30.00f, 1, 3,
         "历史爱好者", "https://picsum.photos/200/280?random=3",
         "2025-07-20 09:15:00"},

        {4, "经济学原理",                 "曼昆",               "9787301150894",
         "北京大学出版社", "经济管理", "八五成新", 45.00f, 5, 4,
         "经济学霸",   "https://picsum.photos/200/280?random=4",
         "2025-09-10 16:45:00"},

        {5, "高等数学",                   "同济大学数学系",     "9787040396638",
         "高等教育出版社", "教育考试", "六成新",   15.00f, 2, 5,
         "考研党",     "https://picsum.photos/200/280?random=5",
         "2025-06-05 08:00:00"},

        {6, "活着",                       "余华",               "9787530211151",
         "北京十月文艺出版社", "文学小说", "九成新", 18.00f, 4, 6,
         "文艺青年",   "https://picsum.photos/200/280?random=6",
         "2025-09-20 11:30:00"},

        {7, "时间简史",                   "史蒂芬·霍金",       "9787535732309",
         "湖南科学技术出版社", "科学技术", "八成新", 35.00f, 3, 7,
         "科学达人",   "https://picsum.photos/200/280?random=7",
         "2025-10-01 13:00:00"},

        {8, "苏菲的世界",                 "乔斯坦·贾德",       "9787506343671",
         "作家出版社", "人文社科", "九成新",   28.00f, 2, 8,
         "哲学思考者", "https://picsum.photos/200/280?random=8",
         "2025-10-10 15:20:00"}
    };

    printf("[4/4] 插入模拟图书...\n");

    char sql[2048];
    for (int i = 0; i < SEED_COUNT; i++) {
        snprintf(sql, sizeof(sql),
            "INSERT OR IGNORE INTO books "
            "(id, name, author, isbn, publisher, "
            " category, condition, price, stock, status, "
            " user_id, seller_name, image_url, create_time) "
            "VALUES "
            "(%d, '%s', '%s', '%s', '%s', "
            " '%s', '%s', %.2f, %d, 0, "
            " %d, '%s', '%s', '%s');",
            books[i].id,    books[i].name,    books[i].author,
            books[i].isbn,  books[i].publisher,
            books[i].category, books[i].condition,
            books[i].price, books[i].stock,
            books[i].user_id, books[i].seller_name,
            books[i].image_url, books[i].create_time);

        if (exec_sql(db, sql) != 0) {
            fprintf(stderr, "[错误] 插入图书 #%d (%s) 失败\n",
                    books[i].id, books[i].name);
            return -1;
        }

        printf("  -> [%d/%d] %s\n", i + 1, SEED_COUNT, books[i].name);
    }

    printf("  -> %d 本图书已创建\n", SEED_COUNT);
    return 0;
}

/*==========================================================================
 * 验证数据完整性
 *==========================================================================*/

static int verify_data(sqlite3 *db) {
    printf("\n===== 数据验证 =====\n");

    /* 检查图书数量 */
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
            "SELECT COUNT(*) FROM books;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            printf("books 表: %d 条记录 %s\n",
                   count, count >= SEED_COUNT ? "✓" : "✗ 不足!");
        }
        sqlite3_finalize(stmt);
    }

    /* 检查用户数量 */
    if (sqlite3_prepare_v2(db,
            "SELECT COUNT(*) FROM users;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            printf("users 表: %d 条记录 %s\n",
                   count, count >= USER_COUNT ? "✓" : "✗ 不足!");
        }
        sqlite3_finalize(stmt);
    }

    /* 检查用户资料数量 */
    if (sqlite3_prepare_v2(db,
            "SELECT COUNT(*) FROM user_profiles;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            printf("user_profiles 表: %d 条记录 %s\n",
                   count, count >= USER_COUNT ? "✓" : "✗ 不足!");
        }
        sqlite3_finalize(stmt);
    }

    return 0;
}

/*==========================================================================
 * 主函数
 *==========================================================================*/

int main(void) {
    sqlite3 *db = NULL;
    int rc;

    printf("============================================================\n");
    printf("  二手图书交易平台 - 种子数据初始化程序\n");
    printf("============================================================\n\n");

    /* 打开数据库 */
    printf("[0/4] 打开数据库 %s ...\n", DB_NAME);
    rc = sqlite3_open(DB_NAME, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[错误] 无法打开数据库 %s: %s\n",
                DB_NAME, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    printf("  -> 数据库连接成功\n\n");

    /* 启用 WAL 模式和外键 */
    exec_sql(db, "PRAGMA journal_mode=WAL;");
    exec_sql(db, "PRAGMA foreign_keys=ON;");

    /* 执行各步骤 */
    if (create_tables(db)   != 0) goto cleanup;
    if (insert_users(db)    != 0) goto cleanup;
    if (insert_profiles(db) != 0) goto cleanup;
    if (insert_books(db)    != 0) goto cleanup;

    /* 验证数据 */
    verify_data(db);

    printf("\n============================================================\n");
    printf("  初始化完成！共导入 %d 本图书, %d 位用户\n",
           SEED_COUNT, USER_COUNT);
    printf("============================================================\n");

cleanup:
    sqlite3_close(db);
    return (rc == SQLITE_OK) ? 0 : 1;
}