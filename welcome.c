#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/*
 * 校园二手书交易管理系统 - 控制台欢迎界面
 *
 * 功能：
 *   程序启动时显示系统名称、功能简介、开发者信息、主菜单
 *   用户可选择进入系统或退出
 */

/*==========================================================================
 * 控制台颜色辅助
 *==========================================================================*/
static void set_color(int color) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, (WORD)color);
}

static void reset_color(void) {
    set_color(7); /* 默认白色 */
}

/*==========================================================================
 * 界面绘制
 *==========================================================================*/

static void print_banner(void) {
    set_color(11); /* 亮青色 */
    printf("\n");
    printf("  ╔══════════════════════════════════════════════════════════╗\n");
    printf("  ║                                                          ║\n");
    set_color(14); /* 亮黄色 */
    printf("  ║     📚  校园二手书交易管理系统  v2.0                      ║\n");
    set_color(11);
    printf("  ║     Campus Second-hand Book Trading System               ║\n");
    printf("  ║                                                          ║\n");
    printf("  ╠══════════════════════════════════════════════════════════╣\n");
    printf("  ║                                                          ║\n");
    reset_color();
}

static void print_info(void) {
    set_color(10); /* 亮绿色 */
    printf("  ║  【系统简介】                                            ║\n");
    reset_color();
    printf("  ║    本系统是一个基于 C 语言开发的校园二手书交易平台，      ║\n");
    printf("  ║    旨在为在校学生提供一个便捷、安全的二手教材交易渠道。  ║\n");
    printf("  ║    学生可以在平台上发布闲置教材、浏览和购买所需书籍、      ║\n");
    printf("  ║    管理个人交易记录，促进校园内图书资源的循环利用。        ║\n");
    printf("  ║                                                          ║\n");

    set_color(10);
    printf("  ║  【主要功能】                                            ║\n");
    reset_color();
    printf("  ║    A. 用户注册与登录        - 身份认证管理                ║\n");
    printf("  ║    B. 构建动态链表          - 从文件/数据库读入数据       ║\n");
    printf("  ║    C. 发布图书信息          - 自动生成编号与发布时间      ║\n");
    printf("  ║    D. 购买图书              - 按编号/书名查找并交易       ║\n");
    printf("  ║    E. 下架/撤销发布         - 卖家验证身份后下架          ║\n");
    printf("  ║    F. 多条件图书查询        - 按书名/作者/ISBN/价格/状态  ║\n");
    printf("  ║    G. 统计与输出报表        - 交易数据统计与分析          ║\n");
    printf("  ║    H. 数据文件保存          - 链表数据持久化存储          ║\n");
    printf("  ║                                                          ║\n");

    set_color(10);
    printf("  ║  【技术架构】                                            ║\n");
    reset_color();
    printf("  ║    开发语言: C 语言                                       ║\n");
    printf("  ║    数据存储: SQLite 数据库 + 动态链表                     ║\n");
    printf("  ║    通信协议: HTTP (基于 Windows Socket API)              ║\n");
    printf("  ║    数据格式: JSON                                        ║\n");
    printf("  ║    服务端口: 8080                                        ║\n");
    printf("  ║                                                          ║\n");

    set_color(10);
    printf("  ║  【开发者信息】                                          ║\n");
    reset_color();
    printf("  ║    开发者: 欧阳周翔                                       ║\n");
    printf("  ║    开发日期: 2026年5月                                   ║\n");
    printf("  ║                                                          ║\n");

    set_color(11);
    printf("  ╚══════════════════════════════════════════════════════════╝\n");
    reset_color();
}

static void print_menu(void) {
    printf("\n");
    set_color(14); /* 亮黄色 */
    printf("  ┌──────────────── 主 菜 单 ────────────────┐\n");
    printf("  │                                            │\n");
    reset_color();
    printf("  │    [1]  一键启动（启动服务器并打开浏览器）   │\n");
    printf("  │    [2]  打印本学期交易情况统计报表           │\n");
    printf("  │    [0]  退出系统                             │\n");
    set_color(14);
    printf("  │                                            │\n");
    printf("  └────────────────────────────────────────────┘\n");
    reset_color();
    printf("\n");
    set_color(15);
    printf("  请选择操作 [0-2]: ");
    reset_color();
}

/*==========================================================================
 * 菜单功能实现
 *==========================================================================*/

static void do_start_server(void) {
    printf("\n");
    set_color(11);
    printf("  ⚡ 正在启动 HTTP 服务器...\n");
    printf("  ────────────────────────────────────────────────\n\n");
    reset_color();

    /* 异步启动 server.exe（非阻塞，后台运行） */
    ShellExecute(NULL, "open", "server.exe", NULL, NULL, SW_HIDE);
}

static void do_open_browser(void) {
    printf("\n");
    set_color(10);
    printf("  🌐 正在打开浏览器...\n");
    reset_color();
    ShellExecute(NULL, "open", "http://localhost:8080", NULL, NULL, SW_SHOW);
    printf("  如果浏览器未自动打开，请手动访问: http://localhost:8080\n");
}

static void do_print_stats_report(void) {
    printf("\n");
    set_color(11);
    printf("  ⚡ 正在生成统计报表...\n");
    printf("  ────────────────────────────────────────────────\n\n");
    reset_color();

    /* 调用 stats_report.exe 显示统计报表 */
    system("stats_report.exe");
}

static void do_show_stats(void) {
    printf("\n");
    set_color(11);
    printf("  ──────────── 数据库统计信息 ────────────\n\n");
    reset_color();

    /* 调用 test.exe 显示统计（如果存在） */
    system("test.exe");
}

static void do_show_help(void) {
    printf("\n");
    set_color(11);
    printf("  ──────────── 帮助文档 ────────────\n\n");
    reset_color();

    printf("  【API 接口列表】\n");
    printf("  ┌─────────────────────────────────────────────────────────┐\n");
    printf("  │ 方法  路径                         说明                 │\n");
    printf("  ├─────────────────────────────────────────────────────────┤\n");
    printf("  │ POST  /api/users/register         用户注册              │\n");
    printf("  │ POST  /api/users/login            用户登录              │\n");
    printf("  │ GET   /api/users/profile?userId=X  获取用户资料         │\n");
    printf("  │ POST  /api/users/profile          更新用户资料          │\n");
    printf("  │ GET   /api/books                    获取所有在售图书     │\n");
    printf("  │ GET   /api/books/{id}              获取图书详情          │\n");
    printf("  │ POST  /api/books                    发布图书              │\n");
    printf("  │ GET   /api/books/search?...         多条件搜索图书       │\n");
    printf("  │ POST  /api/books/{id}/delist        下架图书              │\n");
    printf("  │ GET   /api/books/published?userId=X 查看已发布            │\n");
    printf("  │ GET   /api/books/purchased?userId=X 查看已购买            │\n");
    printf("  │ POST  /api/purchase                 购买图书              │\n");
    printf("  │ POST  /api/collect                  收藏图书              │\n");
    printf("  │ GET   /api/books/collect?userId=X   查看收藏              │\n");
    printf("  │ GET   /api/stats                    统计报表              │\n");
    printf("  └─────────────────────────────────────────────────────────┘\n");

    printf("\n  【搜索接口参数】\n");
    printf("  /api/books/search?keyword=关键词&author=作者&isbn=ISBN\n");
    printf("                   &minPrice=最低价&maxPrice=最高价\n");
    printf("                   &sellerId=卖家ID&status=状态&category=分类\n");
    printf("\n  状态码: 0=在售  1=已售  2=已下架\n\n");
}

/*==========================================================================
 * 主函数
 *==========================================================================*/

int main(void) {
    /* 设置控制台标题 */
    SetConsoleTitle("校园二手书交易管理系统 v2.0");

    /* 设置控制台编码为 UTF-8 */
    SetConsoleOutputCP(65001);

    int choice = -1;

    while (choice != 0) {
        system("cls"); /* 清屏 */

        print_banner();
        print_info();
        print_menu();

        char input[16];
        if (fgets(input, sizeof(input), stdin)) {
            choice = atoi(input);
        }

        switch (choice) {
            case 1:
                do_start_server();
                printf("\n  服务器启动完成！正在打开浏览器...\n");
                Sleep(500);
                do_open_browser();
                printf("\n  按回车键返回主菜单...");
                getchar();
                break;
            case 2:
                do_print_stats_report();
                break;
            case 0:
                set_color(10);
                printf("\n  感谢使用校园二手书交易管理系统，再见！\n\n");
                reset_color();
                break;
            default:
                set_color(12); /* 红色 */
                printf("\n  ⚠ 无效选项，请重新选择！\n");
                reset_color();
                Sleep(1000);
                break;
        }
    }

    return 0;
}