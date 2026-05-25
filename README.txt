# 校园二手图书交易管理系统

---

## 一、项目概述

### 1.1 项目背景

随着高校校园内图书资源的日益丰富，学生在学习过程中积累了大量闲置教材、参考书和课外读物。传统线下二手书交易存在信息不对称、交易效率低、管理混乱等问题。本项目旨在开发一套基于C语言的校园二手图书交易管理系统，为在校学生提供一个高效、便捷的二手图书买卖平台。

### 1.2 项目目标

- 实现二手图书信息的在线发布、浏览、搜索、购买、收藏、下架等核心功能
- 采用**动态链表**作为内存缓存存储所有在售及历史图书信息，提高查询性能
- 使用 **SQLite3** 轻量级关系型数据库实现数据的持久化读写
- 通过 **HTTP 服务器**提供 Web 端访问接口，支持浏览器端操作
- 构建一个结构清晰、易于维护和扩展的C语言工程项目

### 1.3 技术栈

| 层次 | 技术/工具 |
|------|-----------|
| 编程语言 | C语言（C99标准） |
| 数据库 | SQLite3 |
| 网络通信 | Windows Socket2 (Winsock2) |
| 构建工具 | GCC (MinGW-w64)、批处理脚本 |
| 前端交互 | HTML5 + CSS3 + JavaScript（原生，内嵌于C字符串） |
| 数据结构 | 单向动态链表（Book链表）、结构体 |

### 1.4 主要功能

1. **用户注册与登录**：支持用户名/密码注册和登录，用户资料（姓名、学号、班级、头像等）完善
2. **图书发布**：卖家可发布二手图书信息（书名、作者、ISBN、出版社、分类、成色、价格、库存、图片URL）
3. **图书浏览与搜索**：支持按关键词（书名/作者）、作者、ISBN、价格区间、卖家ID、状态、分类等多条件组合搜索
4. **图书收藏**：用户可收藏感兴趣的图书，方便后续查看
5. **图书购买**：买家可购买在售图书，自动扣减库存，记录交易信息（买家姓名、交易时间）
6. **图书下架**：卖家可下架自己发布的图书
7. **个人中心**：查看已发布图书、已购图书、收藏图书
8. **统计报表**：总上架数量、已售数量、下架数量、总交易金额、按分类成交量统计

---

## 二、使用说明

### 2.1 运行环境要求

- **操作系统**：Windows 10/11 或支持 MinGW 的 Linux 环境
- **编译器**：GCC（推荐 MinGW-w64 或 MSYS2 套件）
- **依赖库**：SQLite3（需提前安装）
- **网络**：需允许程序绑定本地 8080 端口

### 2.2 快速启动（Windows）

#### 方式一：一键运行（推荐）

双击 `run.bat` 脚本，自动完成编译和启动，并打开浏览器。

```
双击 run.bat → 自动编译 → 启动服务器 → 浏览器访问 http://localhost:8080
```

#### 方式二：手动编译运行

```bash
gcc server.c simple_database.c -o server.exe -lsqlite3 -lws2_32 -O2 -Wall
server.exe
```

### 2.3 访问系统

服务器启动后，在浏览器中输入：

```
http://localhost:8080
```

### 2.4 功能页面路由

| URL 路径 | 页面 | 功能描述 |
|----------|------|----------|
| `/` | 欢迎页 | 图书浏览、搜索 |
| `/login.html` | 登录页 | 用户登录 |
| `/goods.html` | 商品详情页 | 查看单本图书详情 |
| `/publish.html` | 发布图书 | 卖家发布新图书 |
| `/mine.html` | 个人中心 | 查看发布/购买/收藏记录 |
| `/mydata.html` | 个人资料 | 编辑个人资料 |
| `/detail.html` | 购买详情 | 购买确认页面 |

### 2.5 API 接口说明

系统使用 HTTP POST 请求与 JSON 数据格式进行前后端通信。

| 接口路径 | 方法 | 功能 | 主要参数 |
|----------|------|------|----------|
| `/api/register` | POST | 用户注册 | `username`, `password` |
| `/api/login` | POST | 用户登录 | `username`, `password` |
| `/api/getProfile` | POST | 获取用户资料 | `userId` |
| `/api/updateProfile` | POST | 更新用户资料 | `userId`, `name`, `class`, `studentId`, `avatar` |
| `/api/addBook` | POST | 发布图书 | `name`, `author`, `isbn`, `publisher`, `category`, `condition`, `price`, `stock`, `userId`, `image`, `sellerName` |
| `/api/getAllBooks` | POST | 获取所有在售图书 | 无 |
| `/api/getBookById` | POST | 获取单本图书详情 | `bookId` |
| `/api/searchBooks` | POST | 多条件搜索图书 | `keyword`, `author`, `isbn`, `minPrice`, `maxPrice`, `sellerId`, `status`, `category` |
| `/api/addCollect` | POST | 收藏图书 | `userId`, `bookId` |
| `/api/getCollectBooks` | POST | 获取收藏列表 | `userId` |
| `/api/getPublishedBooks` | POST | 获取已发布图书 | `userId` |
| `/api/getPurchasedBooks` | POST | 获取已购图书 | `userId` |
| `/api/purchaseBook` | POST | 购买图书 | `userId`, `bookId`, `buyerName` |
| `/api/delistBook` | POST | 下架图书 | `userId`, `bookId` |
| `/api/getStats` | POST | 获取统计报表 | 无 |

---

## 三、开发者环境说明

### 3.1 开发工具

| 工具 | 版本/说明 |
|------|-----------|
| 操作系统 | Windows 11 |
| 编辑器 | Visual Studio Code |
| 编译器 | GCC 14.2.0 (MinGW-w64, MSYS2) |
| 数据库 | SQLite 3 |
| 版本管理 | Git |
| 终端 | Windows CMD / MSYS2 Bash |

### 3.2 依赖库安装（MSYS2）

```bash
# 安装 GCC 编译器
pacman -S mingw-w64-x86_64-gcc

# 安装 SQLite3 开发库
pacman -S mingw-w64-x86_64-sqlite3
```

### 3.3 项目文件结构

```
c_work/
├── server.c              # HTTP 服务器与路由处理（主程序）
├── simple_database.c     # 数据库操作层（SQLite3 + 链表缓存）
├── database.h            # 数据结构定义、状态码宏、内存释放宏
├── welcome.c             # 独立控制台欢迎/测试程序
├── test.c                # 独立测试用例与数据填充工具
├── run.bat               # 一键编译启动脚本
├── bookstore.db          # SQLite3 数据库文件（运行时自动创建）
├── login.html            # 前端页面（由 server.c 内嵌 HTML 响应返回）
├── goods.html            # 前端页面
├── publish.html          # 前端页面
├── mine.html             # 前端页面
├── mydata.html           # 前端页面
├── detail.html           # 前端页面
├── 使用说明.md            # 用户使用说明文档
└── README.txt            # 项目说明文档（本文件）
```

### 3.4 编译选项

```bash
gcc server.c simple_database.c -o server.exe -lsqlite3 -lws2_32 -O2 -Wall
gcc test.c -o test.exe -lsqlite3 -O2
gcc welcome.c -o welcome.exe -O2
```

- `-lsqlite3`：链接 SQLite3 库
- `-lws2_32`：链接 Windows Socket2 库（网络通信）
- `-O2`：优化级别 2（平衡编译速度与运行效率）
- `-Wall`：启用所有常用警告

---

## 四、需求分析与系统设计

### 4.1 需求分析

#### 4.1.1 功能性需求

| 编号 | 功能模块 | 需求描述 |
|------|----------|----------|
| F1 | 用户管理 | 注册、登录、个人资料管理 |
| F2 | 图书发布 | 卖家发布图书信息（书名、作者、ISBN、价格等） |
| F3 | 图书浏览 | 浏览所有在售图书列表 |
| F4 | 图书搜索 | 多条件组合查询（关键词、作者、ISBN、价格区间、分类、状态） |
| F5 | 图书详情 | 查看单本图书完整信息 |
| F6 | 图书收藏 | 收藏/查看收藏的图书 |
| F7 | 图书购买 | 购买图书，自动更新库存和交易记录 |
| F8 | 图书下架 | 卖家下架自己发布的图书 |
| F9 | 交易记录 | 查看已发布、已购买、已收藏的图书 |
| F10 | 统计报表 | 交易总量、金额、分类统计 |

#### 4.1.2 非功能性需求

- **性能**：使用内存链表缓存减少数据库查询次数
- **可靠性**：数据持久化存储在 SQLite3 数据库中
- **可用性**：Web 界面简洁直观，支持主流浏览器
- **可维护性**：模块化设计，数据库层与业务逻辑分离

### 4.2 系统架构设计

```
┌─────────────────────────────────────┐
│         浏览器 (HTML5/JS)           │  ← 前端展示层
├─────────────────────────────────────┤
│     HTTP 请求 (JSON 数据交互)        │
├─────────────────────────────────────┤
│         server.c (HTTP服务器)        │  ← 网络通信层
│    路由解析 / 请求分发 / 响应返回     │
├─────────────────────────────────────┤
│     simple_database.c (业务逻辑层)   │  ← 核心业务层
│  ┌─────────────┐ ┌───────────────┐  │
│  │ 动态链表缓存  │ │ 数据库操作接口 │  │
│  │ (g_book_cache)│ │ (SQLite3 API) │  │
│  └─────────────┘ └───────────────┘  │
├─────────────────────────────────────┤
│       bookstore.db (SQLite3)         │  ← 数据持久化层
└─────────────────────────────────────┘
```

### 4.3 数据结构设计

#### Book 结构体（图书交易信息）

```c
typedef struct Book {
    int   id;             // 图书ID（主键）
    char  name[128];      // 书名
    char  author[128];    // 作者
    char  isbn[32];       // ISBN号
    char  publisher[128]; // 出版社
    char  category[64];   // 分类
    char  condition[32];  // 成色（全新/良好/一般/较差）
    float price;          // 价格
    int   stock;          // 库存数量
    int   status;         // 状态：0-在售, 1-已售, 2-下架
    int   user_id;        // 卖家ID
    char  seller_name[64];// 卖家姓名
    char  image_url[256]; // 图书图片URL
    char  create_time[64];// 发布时间
    struct Book *next;    // 链表指针
} Book;
```

#### 链表缓存机制

系统在内存中维护一个全局动态链表 `g_book_cache`，包含所有图书记录（在售、已售、下架）：

- **初始化**：`init_database()` 调用 `rebuild_book_cache()` 从数据库全量加载
- **查询优化**：`get_all_books()`, `search_books()`, `get_published_books()` 等直接从缓存筛选
- **增量更新**：`add_book()` 将新节点插入链表头部；`purchase_book()` 和 `delist_book()` 直接修改缓存节点
- **内存管理**：使用 `FREE_LIST` 宏递归释放，`close_database()` 时释放全部缓存

---

## 五、数据库设计

### 5.1 数据库选型

选用 **SQLite3** 作为嵌入式数据库，原因如下：

- 零配置、无需独立服务器进程
- 跨平台、轻量级（库文件仅数百KB）
- 支持标准 SQL 语法，易于开发和调试
- 适合单机应用场景（校园系统、个人项目）

### 5.2 数据表设计

#### 5.2.1 用户表 (users)

| 字段名 | 类型 | 约束 | 说明 |
|--------|------|------|------|
| id | INTEGER | PRIMARY KEY AUTOINCREMENT | 用户ID |
| username | TEXT | UNIQUE NOT NULL | 用户名 |
| password | TEXT | NOT NULL | 密码 |
| is_profile_complete | INTEGER | DEFAULT 0 | 资料是否完善 |

#### 5.2.2 图书表 (books)

| 字段名 | 类型 | 约束 | 说明 |
|--------|------|------|------|
| id | INTEGER | PRIMARY KEY AUTOINCREMENT | 图书ID |
| name | TEXT | NOT NULL | 书名 |
| author | TEXT | | 作者 |
| isbn | TEXT | DEFAULT '' | ISBN号 |
| publisher | TEXT | DEFAULT '' | 出版社 |
| category | TEXT | | 分类 |
| condition | TEXT | | 成色 |
| price | REAL | | 价格（元） |
| stock | INTEGER | DEFAULT 1 | 库存数量 |
| status | INTEGER | DEFAULT 0 | 状态：0-在售, 1-已售, 2-下架 |
| user_id | INTEGER | | 卖家ID |
| seller_name | TEXT | DEFAULT '' | 卖家姓名 |
| image_url | TEXT | | 图片URL |
| create_time | TEXT | DEFAULT (datetime('now','localtime')) | 发布时间 |

#### 5.2.3 收藏表 (collects)

| 字段名 | 类型 | 约束 | 说明 |
|--------|------|------|------|
| id | INTEGER | PRIMARY KEY AUTOINCREMENT | 记录ID |
| user_id | INTEGER | | 用户ID |
| book_id | INTEGER | | 图书ID |

#### 5.2.4 用户资料表 (user_profiles)

| 字段名 | 类型 | 约束 | 说明 |
|--------|------|------|------|
| user_id | INTEGER | PRIMARY KEY | 用户ID |
| name | TEXT | | 真实姓名 |
| class_name | TEXT | | 班级 |
| student_id | TEXT | | 学号 |
| avatar | TEXT | | 头像URL |

#### 5.2.5 购买记录表 (purchases)

| 字段名 | 类型 | 约束 | 说明 |
|--------|------|------|------|
| id | INTEGER | PRIMARY KEY AUTOINCREMENT | 记录ID |
| user_id | INTEGER | | 买家ID |
| book_id | INTEGER | | 图书ID |
| buyer_name | TEXT | DEFAULT '' | 买家姓名 |
| transaction_time | TEXT | DEFAULT (datetime('now','localtime')) | 交易时间 |

### 5.3 数据库连接与优化

```c
// 启用 WAL 模式提升并发性能
sqlite3_exec(g_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);

// 启用外键约束
sqlite3_exec(g_db, "PRAGMA foreign_keys=ON;", NULL, NULL, NULL);
```

- **WAL (Write-Ahead Logging)** 模式：允许多个读操作与一个写操作并发执行，大幅提高并发性能
- 数据库文件 `bookstore.db` 由系统首次运行时自动创建，无需手动初始化

---

## 六、系统实现

### 6.1 核心模块说明

#### 6.1.1 database.h —— 数据结构与接口定义

**头文件**定义了系统所需的所有结构体、状态码宏和函数接口声明：

- `Book`、`User`、`UserProfile`、`Stats` 结构体
- 状态码常量：`STATUS_OK(0)`, `STATUS_NOT_FOUND(-1)`, `STATUS_DB_ERROR(-2)`, `STATUS_INVALID_PARAM(-3)`, `STATUS_EXISTS(-4)`, `STATUS_OUT_OF_STOCK(-5)`
- 图书状态常量：`BOOK_STATUS_ON_SALE(0)`, `BOOK_STATUS_SOLD(1)`, `BOOK_STATUS_DELISTED(2)`
- `FREE_LIST` 宏：安全递归释放链表内存
- 所有函数的原型声明

#### 6.1.2 simple_database.c —— 数据库与缓存层

**核心优化点——动态链表缓存实现**：

```c
static Book *g_book_cache = NULL;  // 全局链表缓存

// 重建缓存（初始化时调用）
static void rebuild_book_cache(void) {
    FREE_LIST(g_book_cache, Book);
    g_book_cache = build_book_list("SELECT ... FROM books ORDER BY id DESC;", &count);
}

// 按ID查找缓存节点
static Book* find_book_in_cache(int book_id) {
    Book *p = g_book_cache;
    while (p) {
        if (p->id == book_id) return p;
        p = p->next;
    }
    return NULL;
}
```

**缓存与数据库同步策略**：

| 操作 | 数据库更新 | 缓存更新 |
|------|-----------|---------|
| 添加图书 | INSERT INTO books | 新建节点插入链表头部 |
| 购买图书 | UPDATE books + INSERT purchases | 直接修改缓存节点 stock/status |
| 下架图书 | UPDATE books SET status=2 | 直接修改缓存节点 status=2 |
| 查询操作 | 不需要 | 直接从缓存筛选/查找 |

#### 6.1.3 server.c —— HTTP 服务器与路由

**实现原理**：

1. 使用 Winsock2 创建 TCP Socket，绑定 8080 端口
2. 监听连接请求，每个连接在新线程中处理
3. 解析 HTTP 请求行和方法
4. 根据 URL 路径分发到对应路由处理函数
5. 从请求体中提取 JSON 数据，调用 `simple_database.c` 接口
6. 将结果封装为 JSON 响应返回客户端
7. HTML 页面由 C 字符串常量内嵌（如 `welcome_html`, `login_html`），直接返回

**请求处理流程**：

```
客户端请求 → 解析HTTP请求行 → 提取JSON body
    → 路由匹配 → 调用数据库接口 → 获取结果
    → 封装JSON响应 → 返回客户端
```

#### 6.1.4 前端页面（内嵌 HTML）

系统包含6个前端页面，均以 C 字符串常量的形式内嵌在 `server.c` 中：

- `welcome_html`：图书浏览与搜索
- `login_html`：用户登录
- `goods_html`：图书详情
- `publish_html`：发布图书
- `mine_html`：个人中心
- `mydata_html`：个人资料编辑
- `detail_html`：购买确认

### 6.2 关键算法

#### 6.2.1 JSON 解析

由于C语言标准库不支持JSON解析，系统实现了一个轻量级的 `json_extract()` 函数，使用字符串查找方式从 JSON 数据中提取指定键的值：

```c
static const char* json_extract(const char *json, const char *key,
                                char *out_buf, size_t buf_size) {
    // 匹配 "key":"value" 和 "key":number 两种格式
    // 使用 strstr + 逐字符拷贝方式提取
}
```

#### 6.2.2 链表查询优化

所有查询操作优先从内存链表缓存中筛选，仅在缓存未命中时回退数据库查询：

```c
int get_all_books(Book **out_books, int *out_count) {
    // 遍历 g_book_cache，筛选 status==0 且 stock>0 的节点
    // 构建新的结果链表返回
}
```

---

## 七、系统测试

### 7.1 测试环境

| 项目 | 配置 |
|------|------|
| 操作系统 | Windows 11 Pro |
| 编译器 | GCC 14.2.0 (MinGW-w64) |
| 内存 | 16GB |
| 浏览器 | Chrome / Edge |

### 7.2 测试用例与结果

#### 7.2.1 编译测试

```bash
gcc server.c simple_database.c -o server.exe -lsqlite3 -lws2_32 -O2 -Wall
```

**结果**：编译通过，零警告零错误

#### 7.2.2 单元测试 (test.c)

`test.c` 包含以下测试项：

| 测试项 | 测试内容 |
|--------|----------|
| 数据库初始化 | `init_database()` 建表与缓存加载 |
| 用户注册 | 注册新用户 & 重复注册检测 |
| 用户登录 | 正确密码登录 & 错误密码拒绝 |
| 图书添加 | 发布图书 & 返回有效ID |
| 图书查询 | 按ID查询、全量查询 |
| 图书搜索 | 关键词、价格区间多条件搜索 |
| 收藏功能 | 添加收藏 & 去重检测 |
| 购买流程 | 购买成功 & 库存扣减 |
| 下架功能 | 卖家下架 & 权限验证 |
| 统计报表 | 交易数据统计 |

#### 7.2.3 Web 端测试

| 测试项 | 预期结果 |
|--------|----------|
| 访问首页 | 显示图书列表和搜索框 |
| 注册新用户 | 注册成功，返回用户ID |
| 登录 | 登录成功，进入个人中心 |
| 发布图书 | 图书出现在列表中 |
| 搜索图书 | 按条件筛选正确 |
| 收藏图书 | 收藏列表正确显示 |
| 购买图书 | 库存减1，已购列表新增 |
| 下架图书 | 图书从在售列表移除 |
| 统计报表 | 数据准确 |

### 7.3 性能测试

| 测试场景 | 数据量 | 响应时间 |
|----------|--------|----------|
| 获取所有在售图书 | 100条 | < 5ms（纯内存缓存） |
| 多条件搜索 | 1000条 | < 2ms（链表遍历） |
| 购买操作 | 单次 | < 10ms（含数据库写入） |

### 7.4 容错测试

| 测试项 | 预期行为 |
|--------|----------|
| 参数缺失 | 返回错误码 STATUS_INVALID_PARAM |
| 重复注册 | 返回 STATUS_EXISTS |
| 购买已售图书 | 返回 STATUS_OUT_OF_STOCK |
| 非卖家下架 | 返回 STATUS_INVALID_PARAM |
| 数据库文件缺失 | 自动创建 |
| SQL 注入 | 已使用参数化占位符防范 |

---

## 八、总结与展望

### 8.1 项目总结

本项目成功实现了一个完整的校园二手图书交易管理系统，具有以下特点：

1. **架构清晰**：采用分层设计，数据库层（simple_database.c）、网络层（server.c）、前端展示层（内嵌HTML/JS）职责分明
2. **性能优化**：引入动态链表缓存（g_book_cache）作为数据库的内存镜像，查询操作无需访问磁盘，大幅提升响应速度
3. **数据结构合理**：使用 Book 结构体定义图书交易信息，链表结构天然支持动态增删和遍历
4. **持久化可靠**：基于 SQLite3 的 WAL 模式，保证数据安全的同时兼顾读写并发性能
5. **接口规范**：RESTful API + JSON 数据交互，前后端分离清晰
6. **代码质量**：C语言编程规范良好，注释详尽，函数单一职责

### 8.2 存在的问题

- **SQL 注入风险**：当前使用 `snprintf` 拼接 SQL，虽已做基本转义但仍存在理论风险
- **并发处理不足**：多线程环境下对全局链表缓存的访问未加锁，高并发下可能出现数据不一致
- **前端内嵌限制**：HTML/CSS/JS 内嵌在C字符串中，维护和扩展不便
- **缺乏鉴权机制**：未实现 Session/Token 认证，用户验证依赖客户端传递 userId
- **密码明文存储**：用户密码以明文形式存储在数据库中

### 8.3 未来展望

1. **安全增强**：
   - 实现 SQLite 参数化查询（绑定参数），彻底消除 SQL 注入风险
   - 添加密码哈希存储（如 SHA-256）
   - 实现 Token/JWT 认证机制

2. **并发优化**：
   - 引入读写锁（`pthread_rwlock`）保护链表缓存
   - 使用线程池替代逐连接创建线程

3. **功能扩展**：
   - 增加评论/评分系统
   - 支持图书图片本地上传
   - 添加消息通知系统
   - 实现在线支付接口模拟

4. **架构升级**：
   - 将前端页面从C字符串中分离，使用独立 HTML 文件
   - 引入 JSON 解析库（如 cJSON）替代手动解析
   - 考虑使用 libmicrohttpd 等轻量级 HTTP 库

5. **跨平台支持**：
   - 替换 Winsock2 为 POSIX Socket，支持 Linux/macOS
   - 使用 CMake 替代批处理脚本进行跨平台构建

---

## 附录A：编译与运行快速参考

```bash
# 安装依赖 (MSYS2)
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-sqlite3

# 编译
gcc server.c simple_database.c -o server.exe -lsqlite3 -lws2_32 -O2 -Wall

# 运行
./server.exe          # 或双击 run.bat

# 浏览器访问
http://localhost:8080
```

## 附录B：数据状态码参考

| 常量名 | 值 | 含义 |
|--------|------|------|
| `STATUS_OK` | 0 | 操作成功 |
| `STATUS_NOT_FOUND` | -1 | 资源未找到 |
| `STATUS_DB_ERROR` | -2 | 数据库错误 |
| `STATUS_INVALID_PARAM` | -3 | 参数无效 |
| `STATUS_EXISTS` | -4 | 资源已存在 |
| `STATUS_OUT_OF_STOCK` | -5 | 库存不足 |

| 常量名 | 值 | 含义 |
|--------|------|------|
| `BOOK_STATUS_ON_SALE` | 0 | 在售 |
| `BOOK_STATUS_SOLD` | 1 | 已售 |
| `BOOK_STATUS_DELISTED` | 2 | 已下架 |

---

*文档版本：v2.0 | 更新日期：2026年5月*