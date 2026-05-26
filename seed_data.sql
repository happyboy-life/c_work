-- ============================================================
-- 二手图书交易平台 - 模拟图书数据种子脚本
-- ============================================================

-- 创建表结构（与 server.c / simple_database.c 一致）
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    is_profile_complete INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS books (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    author TEXT,
    isbn TEXT DEFAULT '',
    publisher TEXT DEFAULT '',
    category TEXT,
    condition TEXT,
    price REAL,
    stock INTEGER DEFAULT 1,
    status INTEGER DEFAULT 0,
    user_id INTEGER,
    seller_name TEXT DEFAULT '',
    image_url TEXT,
    create_time TEXT DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS collects (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    book_id INTEGER
);

CREATE TABLE IF NOT EXISTS user_profiles (
    user_id INTEGER PRIMARY KEY,
    name TEXT,
    class_name TEXT,
    student_id TEXT,
    avatar TEXT
);

CREATE TABLE IF NOT EXISTS purchases (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER,
    book_id INTEGER,
    buyer_name TEXT DEFAULT '',
    transaction_time TEXT DEFAULT (datetime('now','localtime'))
);

-- 插入模拟卖家用户（如果不存在）
INSERT OR IGNORE INTO users (id, username, password, is_profile_complete)
VALUES
    (1, 'bookworm_lee', '123456', 1),
    (2, 'programmer_wang', '123456', 1),
    (3, 'history_fan', '123456', 1),
    (4, 'econ_master', '123456', 1),
    (5, 'exam_warrior', '123456', 1),
    (6, 'art_youth', '123456', 1),
    (7, 'science_geek', '123456', 1),
    (8, 'philosophy_lover', '123456', 1);

-- 插入模拟卖家资料
INSERT OR IGNORE INTO user_profiles (user_id, name, class_name, student_id, avatar)
VALUES
    (1, '书虫小李', '中文系2022级', '2022001', ''),
    (2, '程序员小王', '计算机系2021级', '2021002', ''),
    (3, '历史爱好者', '历史系2022级', '2022003', ''),
    (4, '经济学霸', '经济学院2022级', '2022004', ''),
    (5, '考研党', '数学系2020级', '2020005', ''),
    (6, '文艺青年', '文学院2022级', '2022006', ''),
    (7, '科学达人', '物理系2023级', '2023007', ''),
    (8, '哲学思考者', '哲学系2021级', '2021008', '');

-- ============================================================
-- 插入模拟图书数据（6本 + 额外2本，共8本）
-- ============================================================
INSERT INTO books (id, name, author, isbn, publisher, category, condition, price, stock, status, user_id, seller_name, image_url, create_time)
VALUES
    (1, '百年孤独', '加西亚·马尔克斯', '9787544253994', '南海出版公司', '文学小说', '八成新', 25.00, 3, 0, 1, '书虫小李', 'https://picsum.photos/200/280?random=1', '2025-09-01 10:00:00'),

    (2, 'JavaScript高级程序设计', '马特·弗里斯比', '9787115546081', '人民邮电出版社', '科学技术', '九成新', 55.00, 2, 0, 2, '程序员小王', 'https://picsum.photos/200/280?random=2', '2025-08-15 14:30:00'),

    (3, '人类简史', '尤瓦尔·赫拉利', '9787508647357', '中信出版社', '人文社科', '七成新', 30.00, 1, 0, 3, '历史爱好者', 'https://picsum.photos/200/280?random=3', '2025-07-20 09:15:00'),

    (4, '经济学原理', '曼昆', '9787301150894', '北京大学出版社', '经济管理', '八五成新', 45.00, 5, 0, 4, '经济学霸', 'https://picsum.photos/200/280?random=4', '2025-09-10 16:45:00'),

    (5, '高等数学', '同济大学数学系', '9787040396638', '高等教育出版社', '教育考试', '六成新', 15.00, 2, 0, 5, '考研党', 'https://picsum.photos/200/280?random=5', '2025-06-05 08:00:00'),

    (6, '活着', '余华', '9787530211151', '北京十月文艺出版社', '文学小说', '九成新', 18.00, 4, 0, 6, '文艺青年', 'https://picsum.photos/200/280?random=6', '2025-09-20 11:30:00'),

    (7, '时间简史', '史蒂芬·霍金', '9787535732309', '湖南科学技术出版社', '科学技术', '八成新', 35.00, 3, 0, 7, '科学达人', 'https://picsum.photos/200/280?random=7', '2025-10-01 13:00:00'),

    (8, '苏菲的世界', '乔斯坦·贾德', '9787506343671', '作家出版社', '人文社科', '九成新', 28.00, 2, 0, 8, '哲学思考者', 'https://picsum.photos/200/280?random=8', '2025-10-10 15:20:00');

-- 完成
SELECT 'Seed data inserted successfully!' AS result;
SELECT COUNT(*) AS total_books FROM books;