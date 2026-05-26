#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

/*==========================================================================
 * 统计报表独立程序 —— 从 bookstore.db 读取并打印格式清晰的交易统计报表
 *
 * 由 welcome.exe 通过 system() 调用
 * 编译: gcc stats_report.c -o stats_report.exe -lsqlite3 -O2 -Wall
 *==========================================================================*/

static sqlite3 *g_db = NULL;

/* 安全释放 SQLite 错误信息 */
#define SAFE_FREE_ERR(e)  do { if (e) { sqlite3_free(e); (e) = NULL; } } while(0)

/* 执行简单 SQL，返回 int 结果 */
static int query_int(const char *sql) {
    sqlite3_stmt *stmt = NULL;
    int result = 0;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            result = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return result;
}

/* 执行简单 SQL，返回 float 结果 */
static float query_float(const char *sql) {
    sqlite3_stmt *stmt = NULL;
    float result = 0.0f;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            result = (float)sqlite3_column_double(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return result;
}

/*==========================================================================
 * 打印分隔线
 *==========================================================================*/
static void print_separator(const char *title) {
    printf("\n");
    printf("  ╔══════════════════════════════════════════════════════════════╗\n");
    if (title && title[0]) {
        printf("  ║  %-60s ║\n", title);
        printf("  ╠══════════════════════════════════════════════════════════════╣\n");
    }
}

static void print_separator_bottom(void) {
    printf("  ╚══════════════════════════════════════════════════════════════╝\n");
}

/*==========================================================================
 * 主函数
 *==========================================================================*/
int main(void) {
    /* ---- 设置控制台编码为 UTF-8 ---- */
    system("chcp 65001 >nul 2>nul");

    /* ---- 打开数据库 ---- */
    int rc = sqlite3_open("bookstore.db", &g_db);
    if (rc != SQLITE_OK) {
        printf("\n  [错误] 无法打开数据库 bookstore.db\n");
        printf("  请确保在正确的目录下运行此程序。\n\n");
        printf("  按回车键返回...");
        getchar();
        return 1;
    }

    /* ---- 总上架数量 ---- */
    int total_listed = query_int("SELECT COUNT(*) FROM books;");

    /* ---- 已售数量 (status = 1) ---- */
    int total_sold = query_int("SELECT COUNT(*) FROM books WHERE status = 1;");

    /* ---- 下架数量 (status = 2) ---- */
    int total_delisted = query_int("SELECT COUNT(*) FROM books WHERE status = 2;");

    /* ---- 在售数量 (status = 0) ---- */
    int total_on_sale = query_int("SELECT COUNT(*) FROM books WHERE status = 0;");

    /* ---- 总交易金额 (已售图书的价格总和) ---- */
    float total_amount = query_float(
        "SELECT COALESCE(SUM(price), 0) FROM books WHERE status = 1;");

    /* ---- 总交易笔数 (purchases 表) ---- */
    int total_transactions = query_int("SELECT COUNT(*) FROM purchases;");

    /* ---- 打印报表 ---- */
    print_separator("📊  本学期交易情况统计报表");

    printf("  ║                                                              ║\n");

    /* 概览 */
    printf("  ║  ┌────────────────────────────────────────────────────┐      ║\n");
    printf("  ║  │               📋  图 书 概 览                       │      ║\n");
    printf("  ║  ├────────────────┬───────────────────────────────────┤      ║\n");
    printf("  ║  │  总上架数量     │  %-33d │      ║\n", total_listed);
    printf("  ║  │  在售数量       │  %-33d │      ║\n", total_on_sale);
    printf("  ║  │  已售数量       │  %-33d │      ║\n", total_sold);
    printf("  ║  │  下架数量       │  %-33d │      ║\n", total_delisted);
    printf("  ║  └────────────────┴───────────────────────────────────┘      ║\n");

    printf("  ║                                                              ║\n");

    /* 交易概览 */
    printf("  ║  ┌────────────────────────────────────────────────────┐      ║\n");
    printf("  ║  │               💰  交 易 概 览                       │      ║\n");
    printf("  ║  ├────────────────┬───────────────────────────────────┤      ║\n");
    printf("  ║  │  总交易笔数     │  %-33d │      ║\n", total_transactions);
    printf("  ║  │  总交易金额     │  ¥%-32.2f │      ║\n", total_amount);
    printf("  ║  └────────────────┴───────────────────────────────────┘      ║\n");

    printf("  ║                                                              ║\n");

    /* 按图书类别成交量 */
    printf("  ║  ┌────────────────────────────────────────────────────┐      ║\n");
    printf("  ║  │          📚  按图书类别成交量 (已售)                │      ║\n");
    printf("  ║  ├────────────────┬──────────┬────────────────────────┤      ║\n");

    sqlite3_stmt *stmt = NULL;
    int rc2 = sqlite3_prepare_v2(g_db,
        "SELECT category, COUNT(*) as cnt FROM books "
        "WHERE status = 1 GROUP BY category ORDER BY cnt DESC;",
        -1, &stmt, NULL);

    if (rc2 == SQLITE_OK) {
        int has_data = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *cat = (const char*)sqlite3_column_text(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            if (!cat || !cat[0]) cat = "(未分类)";

            /* 生成进度条 (使用 # 作为进度块) */
            int bar_len = (total_sold > 0) ? (cnt * 20 / total_sold) : 0;
            if (bar_len < 1 && cnt > 0) bar_len = 1;
            char bar[32] = {0};
            int j;
            for (j = 0; j < bar_len && j < 24; j++) bar[j] = '#';
            bar[j] = '\0';

            printf("  ║  │ %-14s │ %4d 册  │ %-22s │      ║\n",
                   cat, cnt, bar);
            has_data = 1;
        }
        if (!has_data) {
            printf("  ║  │ %-37s    │      ║\n", "暂无已售记录");
        }
        sqlite3_finalize(stmt);
    } else {
        printf("  ║  │ %-37s    │      ║\n", "查询失败");
    }

    printf("  ║  └────────────────┴──────────┴────────────────────────┘      ║\n");

    printf("  ║                                                              ║\n");

    /* 按图书类别金额统计 */
    printf("  ║  ┌────────────────────────────────────────────────────┐      ║\n");
    printf("  ║  │          💵  按图书类别交易金额 (已售)              │      ║\n");
    printf("  ║  ├────────────────┬───────────────────────────────────┤      ║\n");

    sqlite3_stmt *stmt2 = NULL;
    int rc3 = sqlite3_prepare_v2(g_db,
        "SELECT category, COALESCE(SUM(price), 0) as total FROM books "
        "WHERE status = 1 GROUP BY category ORDER BY total DESC;",
        -1, &stmt2, NULL);

    if (rc3 == SQLITE_OK) {
        int has_data = 0;
        while (sqlite3_step(stmt2) == SQLITE_ROW) {
            const char *cat = (const char*)sqlite3_column_text(stmt2, 0);
            float amt = (float)sqlite3_column_double(stmt2, 1);
            if (!cat || !cat[0]) cat = "(未分类)";
            printf("  ║  │ %-14s │  ¥%-31.2f │      ║\n", cat, amt);
            has_data = 1;
        }
        if (!has_data) {
            printf("  ║  │ %-37s    │      ║\n", "暂无已售记录");
        }
        sqlite3_finalize(stmt2);
    }

    printf("  ║  └────────────────┴───────────────────────────────────┘      ║\n");

    print_separator_bottom();

    /* ==================================================================
     * 购买记录明细
     * ==================================================================*/
    printf("\n");
    printf("  ╔══════════════════════════════════════════════════════════════╗\n");
    printf("  ║  🛒  购买记录明细 (最近50条)                                  ║\n");
    printf("  ╠══════════════════════════════════════════════════════════════╣\n");
    printf("  ║  序号  买家         书名                      价格   时间              ║\n");
    printf("  ║  ────────────────────────────────────────────────────────────  ║\n");

    sqlite3_stmt *stmt3 = NULL;
    int rc4 = sqlite3_prepare_v2(g_db,
        "SELECT p.id, p.buyer_name, b.name, b.price, p.transaction_time "
        "FROM purchases p LEFT JOIN books b ON p.book_id = b.id "
        "ORDER BY p.id DESC LIMIT 50;",
        -1, &stmt3, NULL);

    if (rc4 == SQLITE_OK) {
        int has_data = 0;
        int seq = 1;
        while (sqlite3_step(stmt3) == SQLITE_ROW) {
            int pid = sqlite3_column_int(stmt3, 0);
            const char *buyer = (const char*)sqlite3_column_text(stmt3, 1);
            const char *bname = (const char*)sqlite3_column_text(stmt3, 2);
            float price = (float)sqlite3_column_double(stmt3, 3);
            const char *time = (const char*)sqlite3_column_text(stmt3, 4);

            if (!buyer || !buyer[0]) buyer = "(未知)";
            if (!bname || !bname[0]) bname = "(已删除)";
            if (!time || !time[0]) time = "--";

            /* 书名截断 */
            char bname_short[25] = {0};
            snprintf(bname_short, sizeof(bname_short), "%s", bname);
            if (strlen(bname) > 22) { bname_short[22] = '.'; bname_short[23] = '.'; bname_short[24] = '\0'; }

            printf("  ║  %-4d  %-10s  %-22s  ¥%-7.2f  %-16s  ║\n",
                   seq, buyer, bname_short, price, time);
            (void)pid;
            has_data = 1;
            seq++;
        }
        if (!has_data) {
            printf("  ║                    暂无购买记录                              ║\n");
        }
        sqlite3_finalize(stmt3);
    } else {
        printf("  ║                    查询失败                                  ║\n");
    }
    printf("  ╚══════════════════════════════════════════════════════════════╝\n");

    /* ==================================================================
     * 发布记录明细
     * ==================================================================*/
    printf("\n");
    printf("  ╔══════════════════════════════════════════════════════════════╗\n");
    printf("  ║  📤  发布记录明细 (最近50条)                                  ║\n");
    printf("  ╠══════════════════════════════════════════════════════════════╣\n");
    printf("  ║  序号  卖家         书名                      价格   状态     时间              ║\n");
    printf("  ║  ────────────────────────────────────────────────────────────  ║\n");

    sqlite3_stmt *stmt4 = NULL;
    int rc5 = sqlite3_prepare_v2(g_db,
        "SELECT id, seller_name, name, price, status, create_time "
        "FROM books ORDER BY id DESC LIMIT 50;",
        -1, &stmt4, NULL);

    if (rc5 == SQLITE_OK) {
        int has_data = 0;
        int seq2 = 1;
        while (sqlite3_step(stmt4) == SQLITE_ROW) {
            int bid = sqlite3_column_int(stmt4, 0);
            const char *seller = (const char*)sqlite3_column_text(stmt4, 1);
            const char *bname = (const char*)sqlite3_column_text(stmt4, 2);
            float price = (float)sqlite3_column_double(stmt4, 3);
            int status = sqlite3_column_int(stmt4, 4);
            const char *time = (const char*)sqlite3_column_text(stmt4, 5);

            if (!seller || !seller[0]) seller = "(未知)";
            if (!bname || !bname[0]) bname = "(无)";
            if (!time || !time[0]) time = "--";

            const char *status_name = "在售";
            if (status == 1) status_name = "已售";
            else if (status == 2) status_name = "已下架";

            /* 书名截断 */
            char bname_short2[25] = {0};
            snprintf(bname_short2, sizeof(bname_short2), "%s", bname);
            if (strlen(bname) > 22) { bname_short2[22] = '.'; bname_short2[23] = '.'; bname_short2[24] = '\0'; }

            printf("  ║  %-4d  %-10s  %-22s  ¥%-7.2f  %-6s  %-16s  ║\n",
                   seq2, seller, bname_short2, price, status_name, time);
            (void)bid;
            has_data = 1;
            seq2++;
        }
        if (!has_data) {
            printf("  ║                    暂无发布记录                              ║\n");
        }
        sqlite3_finalize(stmt4);
    } else {
        printf("  ║                    查询失败                                  ║\n");
    }
    printf("  ╚══════════════════════════════════════════════════════════════╝\n");

    /* ---- 关闭数据库 ---- */
    sqlite3_close(g_db);

    printf("\n  按回车键返回主菜单...");
    getchar();
    return 0;
}