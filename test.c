#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

/*
 * test.c —— 测试数据填充与统计信息展示工具
 *
 * 用法：
 *   test.exe              显示统计信息
 *   test.exe --seed       填充 35 条测试数据（设计文档要求 ≥30 条）
 *   test.exe --reset      重新创建所有表并填充测试数据
 */

/*==========================================================================
 * 辅助函数
 *==========================================================================*/

static int db_exec(sqlite3 *db, const char *sql) {
    char *err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL错误: %s\nSQL: %s\n", err, sql);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

/*==========================================================================
 * 表结构创建
 *==========================================================================*/

static void create_tables(sqlite3 *db) {
    const char *sqls[] = {
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

    printf("[初始化] 创建数据表...\n");
    size_t n = sizeof(sqls) / sizeof(sqls[0]);
    for (size_t i = 0; i < n; i++) {
        db_exec(db, sqls[i]);
    }
    printf("[初始化] 完成！\n\n");
}

/*==========================================================================
 * 填充测试数据（≥30 条图书，多用户，多状态）
 *==========================================================================*/

static void seed_test_data(sqlite3 *db) {
    /* 先检查是否已有数据 */
    sqlite3_stmt *stmt = NULL;
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM books;", -1, &stmt, NULL);
    int existing = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) existing = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (existing > 0) {
        printf("[提示] 数据库已有 %d 条图书记录，跳过填充。\n", existing);
        printf("       如需重新填充，请使用: test.exe --reset\n");
        return;
    }

    printf("═══════════════════════════════════════════\n");
    printf("        开始填充测试数据 (35 条图书)\n");
    printf("═══════════════════════════════════════════\n\n");

    /* === 用户数据 === */
    const char *users[] = {
        "INSERT INTO users (username, password, is_profile_complete) "
        "VALUES ('zhangsan', '123456', 1);",
        "INSERT INTO users (username, password, is_profile_complete) "
        "VALUES ('lisi', '123456', 1);",
        "INSERT INTO users (username, password, is_profile_complete) "
        "VALUES ('wangwu', '123456', 1);",
        "INSERT INTO users (username, password, is_profile_complete) "
        "VALUES ('zhaoliu', '123456', 1);",
        "INSERT INTO users (username, password, is_profile_complete) "
        "VALUES ('testuser', '123456', 0);"
    };

    printf("[用户] 创建 5 个测试用户...\n");
    for (size_t i = 0; i < 5; i++) db_exec(db, users[i]);

    /* === 用户资料 === */
    const char *profiles[] = {
        "INSERT INTO user_profiles (user_id, name, class_name, student_id, avatar) "
        "VALUES (1, '张三', '计算机科学2101', '2021010001', '📘');",
        "INSERT INTO user_profiles (user_id, name, class_name, student_id, avatar) "
        "VALUES (2, '李四', '软件工程2102', '2021010002', '📗');",
        "INSERT INTO user_profiles (user_id, name, class_name, student_id, avatar) "
        "VALUES (3, '王五', '网络工程2103', '2021010003', '📙');",
        "INSERT INTO user_profiles (user_id, name, class_name, student_id, avatar) "
        "VALUES (4, '赵六', '大数据2101', '2021010004', '📕');"
    };

    printf("[资料] 创建 4 个用户资料...\n");
    for (size_t i = 0; i < 4; i++) db_exec(db, profiles[i]);

    /* === 35 条图书数据（多样化的分类、状态、价格） === */

    /* 格式: name | author | isbn | publisher | category | condition | price | stock | status | user_id | seller_name */
    /* status: 0=在售, 1=已售, 2=已下架 */

    struct TestBook {
        const char *name;
        const char *author;
        const char *isbn;
        const char *publisher;
        const char *category;
        const char *condition;
        float price;
        int stock;
        int status;
        int user_id;
        const char *seller_name;
    };

    /* 注意: create_time 使用 SQLite 默认值 datetime('now','localtime') */

    struct TestBook books[] = {
        /* ===== 计算机类 (在售) ===== */
        {"C程序设计语言(第2版)", "Brian W. Kernighan", "9787111193518", "机械工业出版社",
         "计算机科学", "八成新", 25.00, 1, 0, 1, "张三"},
        {"数据结构(C语言版)", "严蔚敏", "9787302055147", "清华大学出版社",
         "计算机科学", "九成新", 30.00, 1, 0, 1, "张三"},
        {"算法导论(第3版)", "Thomas H. Cormen", "9787111407010", "机械工业出版社",
         "计算机科学", "七成新", 45.00, 1, 0, 2, "李四"},
        {"深入理解计算机系统", "Randal E. Bryant", "9787111544937", "机械工业出版社",
         "计算机科学", "九成新", 55.00, 1, 0, 2, "李四"},
        {"计算机网络:自顶向下方法", "James F. Kurose", "9787111599715", "机械工业出版社",
         "计算机网络", "八成新", 40.00, 1, 0, 3, "王五"},
        {"操作系统概念(第9版)", "Abraham Silberschatz", "9787111604365", "机械工业出版社",
         "操作系统", "六成新", 35.00, 1, 0, 3, "王五"},
        {"数据库系统概念(第6版)", "Abraham Silberschatz", "9787111378648", "机械工业出版社",
         "数据库", "九成新", 50.00, 1, 0, 4, "赵六"},
        {"Python编程:从入门到实践", "Eric Matthes", "9787115428028", "人民邮电出版社",
         "编程语言", "全新", 48.00, 1, 0, 4, "赵六"},

        /* ===== 数学类 ===== */
        {"高等数学(第7版)上册", "同济大学数学系", "9787040396614", "高等教育出版社",
         "数学", "七成新", 15.00, 1, 0, 1, "张三"},
        {"线性代数及其应用", "David C. Lay", "9787111481973", "机械工业出版社",
         "数学", "八成新", 28.00, 1, 0, 2, "李四"},
        {"概率论与数理统计", "盛骤", "9787040019681", "高等教育出版社",
         "数学", "七成新", 12.00, 1, 0, 3, "王五"},
        {"离散数学及其应用(第7版)", "Kenneth H. Rosen", "9787111452706", "机械工业出版社",
         "数学", "九成新", 42.00, 1, 0, 4, "赵六"},

        /* ===== 英语类 ===== */
        {"大学英语精读(第3版)1", "董亚芬", "9787544605204", "上海外语教育出版社",
         "英语", "五成新", 8.00, 2, 0, 1, "张三"},
        {"新视野大学英语读写教程2", "郑树棠", "9787560083254", "外语教学与研究出版社",
         "英语", "六成新", 10.00, 1, 0, 2, "李四"},
        {"英语词汇突破5000", "刘毅", "9787560018553", "外语教学与研究出版社",
         "英语", "八成新", 18.00, 1, 0, 3, "王五"},

        /* ===== 电子通信类 ===== */
        {"信号与系统(第2版)", "Alan V. Oppenheim", "9787121194269", "电子工业出版社",
         "电子通信", "七成新", 38.00, 1, 0, 1, "张三"},
        {"模拟电子技术基础(第3版)", "童诗白", "9787040202526", "高等教育出版社",
         "电子通信", "六成新", 22.00, 1, 0, 2, "李四"},
        {"数字电子技术基础(第5版)", "阎石", "9787040189223", "高等教育出版社",
         "电子通信", "八成新", 28.00, 1, 0, 3, "王五"},

        /* ===== 思想政治类 ===== */
        {"马克思主义基本原理概论", "本书编写组", "9787040437515", "高等教育出版社",
         "思想政治", "六成新", 8.00, 1, 0, 4, "赵六"},
        {"毛泽东思想和中国特色社会主义理论体系概论", "本书编写组",
         "9787040437584", "高等教育出版社",
         "思想政治", "七成新", 10.00, 1, 0, 1, "张三"},
        {"思想道德修养与法律基础", "本书编写组", "9787040437591", "高等教育出版社",
         "思想政治", "五成新", 6.00, 1, 0, 2, "李四"},

        /* ===== 文学类 ===== */
        {"围城", "钱钟书", "9787020024975", "人民文学出版社",
         "文学", "九成新", 15.00, 1, 0, 3, "王五"},
        {"活着", "余华", "9787530211533", "北京十月文艺出版社",
         "文学", "八成新", 12.00, 1, 0, 4, "赵六"},
        {"百年孤独", "加西亚·马尔克斯", "9787544253994", "南海出版公司",
         "文学", "七成新", 20.00, 1, 0, 1, "张三"},

        /* ===== 已售状态 (status=1) ===== */
        {"编译原理(第2版)", "Alfred V. Aho", "9787111251218", "机械工业出版社",
         "计算机科学", "八成新", 38.00, 0, 1, 2, "李四"},
        {"软件工程导论(第6版)", "张海藩", "9787302328797", "清华大学出版社",
         "计算机科学", "九成新", 32.00, 0, 1, 1, "张三"},
        {"计算机组成原理(第2版)", "唐朔飞", "9787040223903", "高等教育出版社",
         "计算机科学", "七成新", 28.00, 0, 1, 3, "王五"},
        {"大学物理(第5版)上册", "马文蔚", "9787040196375", "高等教育出版社",
         "物理", "六成新", 18.00, 0, 1, 4, "赵六"},
        {"高等数学(第7版)下册", "同济大学数学系", "9787040396621", "高等教育出版社",
         "数学", "八成新", 16.00, 0, 1, 1, "张三"},
        {"电路(第5版)", "邱关源", "9787040172010", "高等教育出版社",
         "电子通信", "七成新", 25.00, 0, 1, 2, "李四"},

        /* ===== 已下架状态 (status=2) ===== */
        {"人工智能:一种现代方法(第3版)", "Stuart Russell", "9787111552352", "清华大学出版社",
         "人工智能", "九成新", 68.00, 1, 2, 3, "王五"},
        {"计算机图形学(第3版)", "Donald Hearn", "9787111409823", "机械工业出版社",
         "计算机科学", "八成新", 45.00, 1, 2, 4, "赵六"},
        {"数值分析(第10版)", "Richard L. Burden", "9787111453024", "机械工业出版社",
         "数学", "七成新", 42.00, 1, 2, 1, "张三"},
        {"通信原理(第7版)", "樊昌信", "9787118150401", "国防工业出版社",
         "电子通信", "六成新", 30.00, 1, 2, 2, "李四"},
        {"计算机网络安全教程", "王常吉", "9787115412348", "人民邮电出版社",
         "计算机科学", "九成新", 35.00, 1, 2, 3, "王五"},
    };

    int book_count = (int)(sizeof(books) / sizeof(books[0]));
    printf("[图书] 插入 %d 条图书记录...\n", book_count);

    for (int i = 0; i < book_count; i++) {
        char sql[1024];
        snprintf(sql, sizeof(sql),
                 "INSERT INTO books "
                 "(name, author, isbn, publisher, category, condition, "
                 "price, stock, status, user_id, seller_name) "
                 "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', "
                 "%.2f, %d, %d, %d, '%s');",
                 books[i].name, books[i].author, books[i].isbn,
                 books[i].publisher, books[i].category, books[i].condition,
                 books[i].price, books[i].stock, books[i].status,
                 books[i].user_id, books[i].seller_name);
        db_exec(db, sql);
    }

    /* === 购买记录 === */
    const char *purchases[] = {
        "INSERT INTO purchases (user_id, book_id, buyer_name) VALUES (2, 25, '李四');", /* 李四买了编译原理 */
        "INSERT INTO purchases (user_id, book_id, buyer_name) VALUES (3, 26, '王五');", /* 王五买了软件工程 */
        "INSERT INTO purchases (user_id, book_id, buyer_name) VALUES (4, 27, '赵六');", /* 赵六买了计算机组成原理 */
        "INSERT INTO purchases (user_id, book_id, buyer_name) VALUES (1, 28, '张三');", /* 张三买了大学物理 */
        "INSERT INTO purchases (user_id, book_id, buyer_name) VALUES (2, 29, '李四');",
        "INSERT INTO purchases (user_id, book_id, buyer_name) VALUES (3, 30, '王五');",
    };

    printf("[交易] 插入 %zu 条购买记录...\n", sizeof(purchases)/sizeof(purchases[0]));
    for (size_t i = 0; i < sizeof(purchases)/sizeof(purchases[0]); i++) {
        db_exec(db, purchases[i]);
    }

    /* === 收藏数据 === */
    const char *collects[] = {
        "INSERT INTO collects (user_id, book_id) VALUES (2, 1);",
        "INSERT INTO collects (user_id, book_id) VALUES (2, 3);",
        "INSERT INTO collects (user_id, book_id) VALUES (2, 7);",
        "INSERT INTO collects (user_id, book_id) VALUES (3, 2);",
        "INSERT INTO collects (user_id, book_id) VALUES (3, 6);",
        "INSERT INTO collects (user_id, book_id) VALUES (4, 4);",
        "INSERT INTO collects (user_id, book_id) VALUES (4, 8);",
        "INSERT INTO collects (user_id, book_id) VALUES (4, 12);",
        "INSERT INTO collects (user_id, book_id) VALUES (1, 22);", /* 张三收藏了活着 */
        "INSERT INTO collects (user_id, book_id) VALUES (1, 23);",
    };

    printf("[收藏] 插入 %zu 条收藏记录...\n", sizeof(collects)/sizeof(collects[0]));
    for (size_t i = 0; i < sizeof(collects)/sizeof(collects[0]); i++) {
        db_exec(db, collects[i]);
    }

    printf("\n═══════════════════════════════════════════\n");
    printf("        测试数据填充完成！\n");
    printf("═══════════════════════════════════════════\n");
}

/*==========================================================================
 * 展示统计信息
 *==========================================================================*/

static void show_stats(sqlite3 *db) {
    printf("\n");
    printf("═══════════════════════════════════════════\n");
    printf("        校园二手书交易系统 - 数据统计\n");
    printf("═══════════════════════════════════════════\n\n");

    sqlite3_stmt *stmt = NULL;

    /* 用户统计 */
    printf("┌─────────── 用户统计 ───────────┐\n");
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users;", -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        printf("│  注册用户数: %-19d │\n", sqlite3_column_int(stmt, 0));
    sqlite3_finalize(stmt);
    printf("└──────────────────────────────────┘\n\n");

    /* 图书统计 */
    int total = 0, on_sale = 0, sold = 0, delisted = 0;
    float total_amount = 0;

    sqlite3_prepare_v2(db, "SELECT COUNT(*), "
                            "SUM(CASE WHEN status=0 THEN 1 ELSE 0 END), "
                            "SUM(CASE WHEN status=1 THEN 1 ELSE 0 END), "
                            "SUM(CASE WHEN status=2 THEN 1 ELSE 0 END) "
                            "FROM books;", -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total     = sqlite3_column_int(stmt, 0);
        on_sale   = sqlite3_column_int(stmt, 1);
        sold      = sqlite3_column_int(stmt, 2);
        delisted  = sqlite3_column_int(stmt, 3);
    }
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "SELECT COALESCE(SUM(price), 0) FROM books WHERE status=1;",
                       -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW)
        total_amount = (float)sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);

    printf("┌─────────── 图书统计 ───────────┐\n");
    printf("│  📖 总上架数: %-16d │\n", total);
    printf("│  ✅ 在售数量: %-16d │\n", on_sale);
    printf("│  💰 已售数量: %-16d │\n", sold);
    printf("│  🚫 下架数量: %-16d │\n", delisted);
    printf("│  💵 总交易金额: ¥%-13.2f │\n", total_amount);
    printf("└──────────────────────────────────┘\n\n");

    /* 分类统计 */
    printf("┌────────── 按分类成交量 ──────────┐\n");
    sqlite3_prepare_v2(db,
        "SELECT category, COUNT(*) as cnt FROM books "
        "WHERE status = 1 GROUP BY category ORDER BY cnt DESC;",
        -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("│  %-14s: %-17d│\n",
               sqlite3_column_text(stmt, 0),
               sqlite3_column_int(stmt, 1));
    }
    sqlite3_finalize(stmt);
    printf("└──────────────────────────────────┘\n\n");

    /* 在售图书前5本 */
    printf("┌───────── 在售图书预览(前5) ──────┐\n");
    sqlite3_prepare_v2(db,
        "SELECT id, name, price, seller_name FROM books "
        "WHERE status=0 AND stock>0 LIMIT 5;",
        -1, &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("│ #%d %-16s ¥%-6.2f %-6s│\n",
               sqlite3_column_int(stmt, 0),
               sqlite3_column_text(stmt, 1),
               sqlite3_column_double(stmt, 2),
               sqlite3_column_text(stmt, 3));
    }
    sqlite3_finalize(stmt);
    printf("└──────────────────────────────────┘\n\n");
}

/*==========================================================================
 * 主函数
 *==========================================================================*/

int main(int argc, char *argv[]) {
    sqlite3 *db = NULL;

    if (sqlite3_open("bookstore.db", &db) != SQLITE_OK) {
        fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    /* 检查命令行参数 */
    int do_reset = 0, do_seed = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--reset") == 0) do_reset = 1;
        if (strcmp(argv[i], "--seed") == 0) do_seed = 1;
    }

    if (do_reset) {
        printf("[重置] 删除所有表并重新创建...\n");
        db_exec(db, "DROP TABLE IF EXISTS purchases;");
        db_exec(db, "DROP TABLE IF EXISTS collects;");
        db_exec(db, "DROP TABLE IF EXISTS user_profiles;");
        db_exec(db, "DROP TABLE IF EXISTS books;");
        db_exec(db, "DROP TABLE IF EXISTS users;");
        create_tables(db);
        seed_test_data(db);
    } else if (do_seed) {
        create_tables(db);
        seed_test_data(db);
    }

    /* 始终展示统计信息 */
    show_stats(db);

    sqlite3_close(db);

    printf("按任意键退出...\n");
    getchar();

    return 0;
}