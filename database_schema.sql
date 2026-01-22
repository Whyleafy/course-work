-- ============================================================================
-- SQLite DDL для системы управления оптовой торговлей
-- ============================================================================

-- Включение поддержки внешних ключей (выполняется через PRAGMA в DbManager)
-- PRAGMA foreign_keys = ON;

-- ============================================================================
-- ТАБЛИЦА: products (Товары)
-- ============================================================================
CREATE TABLE IF NOT EXISTS products (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    unit TEXT NOT NULL DEFAULT 'кг',
    price TEXT NOT NULL DEFAULT '0.00',
    sort INTEGER NOT NULL DEFAULT 0,
    is_active INTEGER NOT NULL DEFAULT 1 CHECK(is_active IN (0, 1)),
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- ============================================================================
-- ТАБЛИЦА: counterparties (Контрагенты)
-- ============================================================================
CREATE TABLE IF NOT EXISTS counterparties (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    type TEXT NOT NULL CHECK(type IN ('supplier', 'customer', 'both')),
    address TEXT,
    is_active INTEGER NOT NULL DEFAULT 1 CHECK(is_active IN (0, 1)),
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now'))
);

-- ============================================================================
-- ТАБЛИЦА: requisites (Реквизиты контрагентов)
-- ============================================================================
CREATE TABLE IF NOT EXISTS requisites (
    counterparty_id INTEGER NOT NULL,
    inn TEXT,
    kpp TEXT,
    bik TEXT,
    bank_name TEXT,
    r_account TEXT,
    k_account TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now')),
    PRIMARY KEY (counterparty_id),
    FOREIGN KEY (counterparty_id) REFERENCES counterparties(id) ON DELETE CASCADE
);

-- ============================================================================
-- ТАБЛИЦА: documents (Документы)
-- ============================================================================
CREATE TABLE IF NOT EXISTS documents (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    doc_type TEXT NOT NULL CHECK(doc_type IN ('supply', 'sale', 'return', 'transfer')),
    number TEXT NOT NULL,
    date TEXT NOT NULL DEFAULT (date('now')),
    status TEXT NOT NULL DEFAULT 'DRAFT' CHECK(status IN ('DRAFT', 'POSTED', 'CANCELLED')),
    sender_id INTEGER,
    receiver_id INTEGER,
    total_amount TEXT NOT NULL DEFAULT '0.00',
    notes TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now')),
    FOREIGN KEY (sender_id) REFERENCES counterparties(id),
    FOREIGN KEY (receiver_id) REFERENCES counterparties(id),
    UNIQUE(number, doc_type)
);

-- ============================================================================
-- ТАБЛИЦА: document_lines (Строки документов)
-- ============================================================================
CREATE TABLE IF NOT EXISTS document_lines (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    document_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    qty_kg REAL NOT NULL DEFAULT 0.0,
    price TEXT NOT NULL DEFAULT '0.00',
    line_sum TEXT NOT NULL DEFAULT '0.00',
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(id)
);

-- ============================================================================
-- ТАБЛИЦА: inventory_movements (Движения товара)
-- ============================================================================
CREATE TABLE IF NOT EXISTS inventory_movements (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    document_id INTEGER NOT NULL,
    product_id INTEGER NOT NULL,
    qty_delta_kg REAL NOT NULL,
    movement_date TEXT NOT NULL DEFAULT (date('now')),
    cancelled_flag INTEGER NOT NULL DEFAULT 0 CHECK(cancelled_flag IN (0, 1)),
    created_at TEXT NOT NULL DEFAULT (datetime('now')),
    FOREIGN KEY (document_id) REFERENCES documents(id),
    FOREIGN KEY (product_id) REFERENCES products(id)
);

-- ============================================================================
-- ИНДЕКСЫ для производительности
-- ============================================================================

-- Индексы для документов
CREATE INDEX IF NOT EXISTS idx_documents_status ON documents(status);
CREATE INDEX IF NOT EXISTS idx_documents_date ON documents(date);
CREATE INDEX IF NOT EXISTS idx_documents_sender ON documents(sender_id);
CREATE INDEX IF NOT EXISTS idx_documents_receiver ON documents(receiver_id);

-- Индексы для строк документов
CREATE INDEX IF NOT EXISTS idx_document_lines_document ON document_lines(document_id);
CREATE INDEX IF NOT EXISTS idx_document_lines_product ON document_lines(product_id);

-- Индексы для движений товара
CREATE INDEX IF NOT EXISTS idx_movements_document ON inventory_movements(document_id);
CREATE INDEX IF NOT EXISTS idx_movements_product ON inventory_movements(product_id);
CREATE INDEX IF NOT EXISTS idx_movements_date ON inventory_movements(movement_date);
CREATE INDEX IF NOT EXISTS idx_movements_cancelled ON inventory_movements(cancelled_flag);

-- Индексы для контрагентов
CREATE INDEX IF NOT EXISTS idx_counterparties_type ON counterparties(type);
CREATE INDEX IF NOT EXISTS idx_counterparties_active ON counterparties(is_active);

-- Индексы для товаров
CREATE INDEX IF NOT EXISTS idx_products_active ON products(is_active);
CREATE INDEX IF NOT EXISTS idx_products_sort ON products(sort);
