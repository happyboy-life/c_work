#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data_persistence.h"

/*==========================================================================
 * 静态辅助 —— 计算链表长度
 *==========================================================================*/
static int book_list_count(Book *head) {
    int n = 0;
    while (head) { n++; head = head->next; }
    return n;
}

/*==========================================================================
 * save_books_to_file —— 将 Book 链表序列化为二进制文件
 *
 * 文件格式 (小端序):
 *   [4 bytes] magic    = 0x424B4442
 *   [4 bytes] version  = 1
 *   [4 bytes] count    = N (节点数量)
 *   [N * sizeof(Book)] 平坦结构数组
 *   [4 bytes] checksum = 简单异或校验
 *==========================================================================*/
int save_books_to_file(Book *head, const char *path) {
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "[DataFile] 无法写入文件: %s\n", path);
        return STATUS_DB_ERROR;
    }

    int count = book_list_count(head);
    unsigned int magic   = DAT_FILE_MAGIC;
    unsigned int version = DAT_FILE_VERSION;
    unsigned int checksum = 0;

    /* 写文件头 */
    fwrite(&magic,   sizeof(magic),   1, fp);
    fwrite(&version, sizeof(version), 1, fp);
    fwrite(&count,   sizeof(count),   1, fp);

    /* 逐节点写入 (平铺，不包含 next 指针) */
    Book *p = head;
    while (p) {
        size_t written = fwrite(p, sizeof(Book), 1, fp);
        if (written != 1) {
            fprintf(stderr, "[DataFile] 写入节点失败 (id=%d)\n", p->id);
            fclose(fp);
            return STATUS_DB_ERROR;
        }
        /* 累加简单校验和 (对每个字节异或) */
        unsigned char *raw = (unsigned char*)p;
        for (size_t i = 0; i < sizeof(Book); i++)
            checksum ^= raw[i];
        p = p->next;
    }

    /* 写校验和 */
    fwrite(&checksum, sizeof(checksum), 1, fp);
    fclose(fp);

    printf("[DataFile] 已保存 %d 条图书记录到 %s\n", count, path);
    return STATUS_OK;
}

/*==========================================================================
 * load_books_from_file —— 从二进制文件恢复 Book 链表
 *
 * 返回值:
 *   STATUS_OK        —— 加载成功
 *   STATUS_NOT_FOUND  —— 文件不存在
 *   STATUS_DB_ERROR   —— 文件格式损坏
 *==========================================================================*/
int load_books_from_file(Book **out_head, int *out_count, const char *path) {
    if (!out_head || !out_count) return STATUS_DB_ERROR;
    *out_head = NULL;
    *out_count = 0;

    FILE *fp = fopen(path, "rb");
    if (!fp) return STATUS_NOT_FOUND;

    unsigned int magic = 0, version = 0;
    int count = 0;
    unsigned int stored_checksum = 0, computed_checksum = 0;

    /* 读文件头 */
    if (fread(&magic, sizeof(magic), 1, fp) != 1 ||
        fread(&version, sizeof(version), 1, fp) != 1 ||
        fread(&count, sizeof(count), 1, fp) != 1) {
        fprintf(stderr, "[DataFile] 文件头损坏: %s\n", path);
        fclose(fp);
        return STATUS_DB_ERROR;
    }

    if (magic != DAT_FILE_MAGIC || version != DAT_FILE_VERSION) {
        fprintf(stderr, "[DataFile] 文件版本不匹配 (magic=0x%X, ver=%u)\n",
                magic, version);
        fclose(fp);
        return STATUS_DB_ERROR;
    }

    if (count < 0 || count > 1000000) {
        fprintf(stderr, "[DataFile] 记录数异常: %d\n", count);
        fclose(fp);
        return STATUS_DB_ERROR;
    }

    /* 逐个读取节点 */
    Book *head = NULL, *tail = NULL;
    for (int i = 0; i < count; i++) {
        Book *node = (Book*)calloc(1, sizeof(Book));
        if (!node) {
            FREE_LIST(head, Book);
            fclose(fp);
            return STATUS_DB_ERROR;
        }

        if (fread(node, sizeof(Book), 1, fp) != 1) {
            fprintf(stderr, "[DataFile] 读取第 %d 个节点失败\n", i);
            free(node);
            FREE_LIST(head, Book);
            fclose(fp);
            return STATUS_DB_ERROR;
        }

        /* 强制清除 next 指针 (文件中的值是废弃的) */
        node->next = NULL;

        /* 计算校验和 */
        unsigned char *raw = (unsigned char*)node;
        for (size_t j = 0; j < sizeof(Book); j++)
            computed_checksum ^= raw[j];

        /* 尾插 */
        if (!head) head = tail = node;
        else { tail->next = node; tail = node; }
    }

    /* 读校验和 */
    if (fread(&stored_checksum, sizeof(stored_checksum), 1, fp) != 1) {
        fprintf(stderr, "[DataFile] 读取校验和失败\n");
        FREE_LIST(head, Book);
        fclose(fp);
        return STATUS_DB_ERROR;
    }

    fclose(fp);

    if (stored_checksum != computed_checksum) {
        fprintf(stderr, "[DataFile] 校验和错误! 期望=0x%X, 实际=0x%X\n",
                stored_checksum, computed_checksum);
        FREE_LIST(head, Book);
        return STATUS_DB_ERROR;
    }

    *out_head  = head;
    *out_count = count;
    printf("[DataFile] 从 %s 加载了 %d 条图书记录\n", path, count);
    return STATUS_OK;
}

/*==========================================================================
 * 用户链表辅助 —— 计算长度
 *==========================================================================*/
static int user_list_count(User *head) {
    int n = 0;
    while (head) { n++; head = head->next; }
    return n;
}

/*==========================================================================
 * save_users_to_file —— 将 User 链表序列化为二进制文件
 *
 * 文件格式与 books.dat 一致:
 *   [4 bytes] magic    = 0x424B4442
 *   [4 bytes] version  = 1
 *   [4 bytes] count    = N
 *   [N * sizeof(User)]  平坦结构数组 (不含 next 指针)
 *   [4 bytes] checksum
 *==========================================================================*/
int save_users_to_file(User *head, const char *path) {
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "[DataFile] 无法写入用户文件: %s\n", path);
        return STATUS_DB_ERROR;
    }

    int count = user_list_count(head);
    unsigned int magic   = DAT_FILE_MAGIC;
    unsigned int version = DAT_FILE_VERSION;
    unsigned int checksum = 0;

    /* 写文件头 */
    fwrite(&magic,   sizeof(magic),   1, fp);
    fwrite(&version, sizeof(version), 1, fp);
    fwrite(&count,   sizeof(count),   1, fp);

    /* 逐节点写入 */
    User *p = head;
    while (p) {
        size_t written = fwrite(p, sizeof(User), 1, fp);
        if (written != 1) {
            fprintf(stderr, "[DataFile] 写入用户节点失败 (id=%d)\n", p->id);
            fclose(fp);
            return STATUS_DB_ERROR;
        }
        unsigned char *raw = (unsigned char*)p;
        for (size_t i = 0; i < sizeof(User); i++)
            checksum ^= raw[i];
        p = p->next;
    }

    /* 写校验和 */
    fwrite(&checksum, sizeof(checksum), 1, fp);
    fclose(fp);

    printf("[DataFile] 已保存 %d 条用户记录到 %s\n", count, path);
    return STATUS_OK;
}

/*==========================================================================
 * load_users_from_file —— 从二进制文件恢复 User 链表
 *==========================================================================*/
int load_users_from_file(User **out_head, int *out_count, const char *path) {
    if (!out_head || !out_count) return STATUS_DB_ERROR;
    *out_head = NULL;
    *out_count = 0;

    FILE *fp = fopen(path, "rb");
    if (!fp) return STATUS_NOT_FOUND;

    unsigned int magic = 0, version = 0;
    int count = 0;
    unsigned int stored_checksum = 0, computed_checksum = 0;

    /* 读文件头 */
    if (fread(&magic, sizeof(magic), 1, fp) != 1 ||
        fread(&version, sizeof(version), 1, fp) != 1 ||
        fread(&count, sizeof(count), 1, fp) != 1) {
        fprintf(stderr, "[DataFile] 用户文件头损坏: %s\n", path);
        fclose(fp);
        return STATUS_DB_ERROR;
    }

    if (magic != DAT_FILE_MAGIC || version != DAT_FILE_VERSION) {
        fprintf(stderr, "[DataFile] 用户文件版本不匹配 (magic=0x%X, ver=%u)\n",
                magic, version);
        fclose(fp);
        return STATUS_DB_ERROR;
    }

    if (count < 0 || count > 1000000) {
        fprintf(stderr, "[DataFile] 用户记录数异常: %d\n", count);
        fclose(fp);
        return STATUS_DB_ERROR;
    }

    /* 逐个读取 */
    User *head = NULL, *tail = NULL;
    for (int i = 0; i < count; i++) {
        User *node = (User*)calloc(1, sizeof(User));
        if (!node) {
            FREE_LIST(head, User);
            fclose(fp);
            return STATUS_DB_ERROR;
        }
        if (fread(node, sizeof(User), 1, fp) != 1) {
            fprintf(stderr, "[DataFile] 读取第 %d 个用户节点失败\n", i);
            free(node);
            FREE_LIST(head, User);
            fclose(fp);
            return STATUS_DB_ERROR;
        }
        node->next = NULL;

        unsigned char *raw = (unsigned char*)node;
        for (size_t j = 0; j < sizeof(User); j++)
            computed_checksum ^= raw[j];

        if (!head) head = tail = node;
        else { tail->next = node; tail = node; }
    }

    /* 校验 */
    if (fread(&stored_checksum, sizeof(stored_checksum), 1, fp) != 1) {
        fprintf(stderr, "[DataFile] 读取用户文件校验和失败\n");
        FREE_LIST(head, User);
        fclose(fp);
        return STATUS_DB_ERROR;
    }

    fclose(fp);

    if (stored_checksum != computed_checksum) {
        fprintf(stderr, "[DataFile] 用户文件校验和错误!\n");
        FREE_LIST(head, User);
        return STATUS_DB_ERROR;
    }

    *out_head  = head;
    *out_count = count;
    printf("[DataFile] 从 %s 加载了 %d 条用户记录\n", path, count);
    return STATUS_OK;
}
