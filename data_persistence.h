#ifndef DATA_PERSISTENCE_H
#define DATA_PERSISTENCE_H

#include "database.h"

/*==========================================================================
 * 数据持久化模块 —— 将链表数据保存到二进制文件
 *
 * 设计目标:
 *   1. 每次增删改后将 Book 链表全量保存到 books.dat
 *   2. 启动时若 SQLite 异常，可从 books.dat 恢复数据
 *   3. 二进制格式紧凑高效，适合频繁读写
 *==========================================================================*/

/* 文件魔法数字 —— 用于校验文件有效性 */
#define DAT_FILE_MAGIC  0x424B4442  /* "BKDB" in little-endian hex */
#define DAT_FILE_VERSION 1

/* 保存整个 Book 链表到文件
 * @param head  链表头指针
 * @param path  文件路径 (如 "books.dat")
 * @return      STATUS_OK / STATUS_DB_ERROR
 */
int save_books_to_file(Book *head, const char *path);

/* 从文件加载 Book 链表
 * @param out_head  输出: 链表头指针 (调用者用 FREE_LIST 释放)
 * @param out_count 输出: 节点数量
 * @param path      文件路径
 * @return          STATUS_OK / STATUS_DB_ERROR / STATUS_NOT_FOUND
 */
int load_books_from_file(Book **out_head, int *out_count, const char *path);

/*==========================================================================
 * 用户数据持久化 —— users.dat
 *==========================================================================*/

/* 保存整个 User 链表到文件 */
int save_users_to_file(User *head, const char *path);

/* 从文件加载 User 链表 */
int load_users_from_file(User **out_head, int *out_count, const char *path);

#endif /* DATA_PERSISTENCE_H */
