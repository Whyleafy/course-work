#include "MigrationRunner.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QLoggingCategory>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>
#include <QIODevice>

Q_LOGGING_CATEGORY(migration, "migration")

MigrationRunner::MigrationRunner(QSqlDatabase db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
}

static bool columnExists(QSqlDatabase& db, const QString& tableName, const QString& columnName)
{
    QSqlQuery q(db);
    q.prepare(QString("PRAGMA table_info(%1)").arg(tableName));
    if (!q.exec()) return false;

    while (q.next()) {
        const QString name = q.value("name").toString();
        if (name.compare(columnName, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool MigrationRunner::runMigrations()
{
    if (!m_db.isOpen()) {
        qCritical(migration) << "MigrationRunner: Database is not open";
        return false;
    }

    if (!m_db.transaction()) {
        qCritical(migration) << "MigrationRunner: Cannot start transaction:" << m_db.lastError().text();
        return false;
    }

    qInfo(migration) << "MigrationRunner: Starting migrations...";

    try {
        bool needsMigration = false;

        QStringList requiredTables = {
            "products",
            "counterparties",
            "requisites",
            "documents",
            "document_lines",
            "inventory_movements"
        };

        for (const QString &tableName : requiredTables) {
            if (!tableExists(tableName)) {
                needsMigration = true;
                qInfo(migration) << "MigrationRunner: Table" << tableName << "does not exist, migration needed";
                break;
            }
        }

        if (needsMigration) {
            if (!createAllTables()) {
                qCritical(migration) << "MigrationRunner: Failed to create tables";
                m_db.rollback();
                return false;
            }

            if (!createIndexes()) {
                qCritical(migration) << "MigrationRunner: Failed to create indexes";
                m_db.rollback();
                return false;
            }

            if (!m_db.commit()) {
                qCritical(migration) << "MigrationRunner: Cannot commit transaction:" << m_db.lastError().text();
                m_db.rollback();
                return false;
            }

            qInfo(migration) << "MigrationRunner: Migrations completed successfully";

            if (!loadTestData()) {
                qWarning(migration) << "MigrationRunner: Failed to load test data";
            } else {
                qInfo(migration) << "MigrationRunner: Test data loaded successfully";
            }

            return true;
        }

        if (!columnExists(m_db, "documents", "is_deleted")) {
            qInfo(migration) << "MigrationRunner: Adding documents.is_deleted column...";

            // SQLite позволяет ADD COLUMN
            if (!executeQuery(
                    "ALTER TABLE documents ADD COLUMN is_deleted INTEGER NOT NULL DEFAULT 0",
                    "alter documents add is_deleted"
                )) {
                qCritical(migration) << "MigrationRunner: Failed to add documents.is_deleted";
                m_db.rollback();
                return false;
            }
        }

        if (!executeQuery(
                "CREATE INDEX IF NOT EXISTS idx_documents_is_deleted ON documents(is_deleted)",
                "createIndexes: documents_is_deleted"
            )) {
            qCritical(migration) << "MigrationRunner: Failed to create idx_documents_is_deleted";
            m_db.rollback();
            return false;
        }

        if (!executeQuery(
                "UPDATE documents "
                "SET number = number || ' [del ' || id || ']' "
                "WHERE is_deleted = 1 AND instr(number, '[del ') = 0",
                "normalize deleted document numbers"
            )) {
            qCritical(migration) << "MigrationRunner: Failed to normalize deleted document numbers";
            m_db.rollback();
            return false;
        }

        if (!m_db.commit()) {
            qCritical(migration) << "MigrationRunner: Cannot commit transaction:" << m_db.lastError().text();
            m_db.rollback();
            return false;
        }

        qInfo(migration) << "MigrationRunner: Migrations completed successfully";
        return true;

    } catch (...) {
        qCritical(migration) << "MigrationRunner: Exception during migration";
        m_db.rollback();
        return false;
    }
}

bool MigrationRunner::tableExists(const QString &tableName)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
    query.addBindValue(tableName);

    if (!query.exec()) {
        qWarning(migration) << "MigrationRunner: Cannot check table existence:" << query.lastError().text();
        return false;
    }

    return query.next();
}

bool MigrationRunner::createAllTables()
{
    return createProductsTable() &&
           createCounterpartiesTable() &&
           createRequisitesTable() &&
           createDocumentsTable() &&
           createDocumentLinesTable() &&
           createInventoryMovementsTable();
}

bool MigrationRunner::createProductsTable()
{
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            unit TEXT NOT NULL DEFAULT 'кг',
            price REAL NOT NULL DEFAULT 0.0,
            sort INTEGER NOT NULL DEFAULT 0,
            is_active INTEGER NOT NULL DEFAULT 1 CHECK(is_active IN (0, 1)),
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now'))
        )
    )";

    return executeQuery(sql, "createProductsTable");
}

bool MigrationRunner::createCounterpartiesTable()
{
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS counterparties (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            type TEXT NOT NULL CHECK(type IN ('supplier', 'customer', 'both')),
            address TEXT,
            is_active INTEGER NOT NULL DEFAULT 1 CHECK(is_active IN (0, 1)),
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now'))
        )
    )";

    return executeQuery(sql, "createCounterpartiesTable");
}

bool MigrationRunner::createRequisitesTable()
{
    QString sql = R"(
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
        )
    )";

    return executeQuery(sql, "createRequisitesTable");
}

bool MigrationRunner::createDocumentsTable()
{
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS documents (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            doc_type TEXT NOT NULL CHECK(doc_type IN ('supply', 'sale', 'return', 'transfer', 'writeoff')),
            number TEXT NOT NULL,
            date TEXT NOT NULL DEFAULT (date('now')),
            status TEXT NOT NULL DEFAULT 'DRAFT' CHECK(status IN ('DRAFT', 'POSTED', 'CANCELLED')),
            sender_id INTEGER,
            receiver_id INTEGER,
            total_amount REAL NOT NULL DEFAULT 0.0,
            notes TEXT,
            is_deleted INTEGER NOT NULL DEFAULT 0 CHECK(is_deleted IN (0, 1)),
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            updated_at TEXT NOT NULL DEFAULT (datetime('now')),
            FOREIGN KEY (sender_id) REFERENCES counterparties(id),
            FOREIGN KEY (receiver_id) REFERENCES counterparties(id),
            UNIQUE(number, doc_type)
        )
    )";

    return executeQuery(sql, "createDocumentsTable");
}

bool MigrationRunner::createDocumentLinesTable()
{
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS document_lines (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            document_id INTEGER NOT NULL,
            product_id INTEGER NOT NULL,
            qty_kg REAL NOT NULL DEFAULT 0.0,
            price REAL NOT NULL DEFAULT 0.0,
            line_sum REAL NOT NULL DEFAULT 0.0,
            created_at TEXT NOT NULL DEFAULT (datetime('now')),
            FOREIGN KEY (document_id) REFERENCES documents(id) ON DELETE CASCADE,
            FOREIGN KEY (product_id) REFERENCES products(id)
        )
    )";

    return executeQuery(sql, "createDocumentLinesTable");
}

bool MigrationRunner::createInventoryMovementsTable()
{
    QString sql = R"(
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
        )
    )";

    return executeQuery(sql, "createInventoryMovementsTable");
}

bool MigrationRunner::createIndexes()
{
    bool success = true;

    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_documents_status ON documents(status)",
        "createIndexes: documents_status"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_documents_date ON documents(date)",
        "createIndexes: documents_date"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_documents_sender ON documents(sender_id)",
        "createIndexes: documents_sender"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_documents_receiver ON documents(receiver_id)",
        "createIndexes: documents_receiver"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_documents_is_deleted ON documents(is_deleted)",
        "createIndexes: documents_is_deleted"
    );

    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_document_lines_document ON document_lines(document_id)",
        "createIndexes: document_lines_document"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_document_lines_product ON document_lines(product_id)",
        "createIndexes: document_lines_product"
    );

    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_movements_document ON inventory_movements(document_id)",
        "createIndexes: movements_document"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_movements_product ON inventory_movements(product_id)",
        "createIndexes: movements_product"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_movements_date ON inventory_movements(movement_date)",
        "createIndexes: movements_date"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_movements_cancelled ON inventory_movements(cancelled_flag)",
        "createIndexes: movements_cancelled"
    );

    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_counterparties_type ON counterparties(type)",
        "createIndexes: counterparties_type"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_counterparties_active ON counterparties(is_active)",
        "createIndexes: counterparties_active"
    );

    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_products_active ON products(is_active)",
        "createIndexes: products_active"
    );
    success &= executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_products_sort ON products(sort)",
        "createIndexes: products_sort"
    );

    return success;
}

bool MigrationRunner::executeQuery(const QString &sql, const QString &errorContext)
{
    QSqlQuery query(m_db);

    if (!query.exec(sql)) {
        QString context = errorContext.isEmpty() ? "executeQuery" : errorContext;
        qCritical(migration) << "MigrationRunner:" << context << "- SQL error:" << query.lastError().text();
        qCritical(migration) << "MigrationRunner:" << context << "- SQL:" << sql;
        return false;
    }

    return true;
}

bool MigrationRunner::loadTestData()
{
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT COUNT(*) FROM products");
    if (checkQuery.exec() && checkQuery.next()) {
        int count = checkQuery.value(0).toInt();
        if (count > 0) {
            qInfo(migration) << "MigrationRunner: Test data already exists, skipping";
            return true;
        }
    }

    qInfo(migration) << "MigrationRunner: Loading test data...";

    QFile sqlFile(":/test_data.sql");
    if (!sqlFile.exists()) {
        QString sqlPath = QCoreApplication::applicationDirPath() + "/../test_data.sql";
        if (!QFile::exists(sqlPath)) {
            sqlPath = QDir::currentPath() + "/test_data.sql";
        }
        sqlFile.setFileName(sqlPath);
    }

    if (!sqlFile.exists() || !sqlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning(migration) << "MigrationRunner: Test data file not found, using inline SQL";
        return loadTestDataInline();
    }

    QTextStream in(&sqlFile);
    QString sql = in.readAll();
    sqlFile.close();

    QStringList statements = sql.split(';', Qt::SkipEmptyParts);

    for (const QString &statement : statements) {
        QString trimmed = statement.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("--")) {
            continue;
        }

        if (!executeQuery(trimmed, "loadTestData")) {
            qWarning(migration) << "MigrationRunner: Failed to execute test data statement";
        }
    }

    return true;
}

bool MigrationRunner::loadTestDataInline()
{
    QStringList statements = {
        R"(INSERT OR IGNORE INTO products (name, unit, price, sort, is_active) VALUES
('Мука пшеничная высший сорт', 'кг', 45.50, 1, 1),
('Мука ржаная обойная', 'кг', 38.00, 2, 1),
('Мука овсяная', 'кг', 52.00, 3, 1),
('Мука кукурузная', 'кг', 48.50, 4, 1),
('Отруби пшеничные', 'кг', 15.00, 5, 1),
('Крупа гречневая ядрица', 'кг', 85.00, 6, 1),
('Крупа рисовая', 'кг', 65.00, 7, 1),
('Крупа овсяная', 'кг', 42.00, 8, 1))",

        R"(INSERT OR IGNORE INTO counterparties (name, type, address, is_active) VALUES
('ООО "Хлебзавод №1"', 'customer', 'г. Москва, ул. Хлебная, д. 10', 1),
('ИП Иванов Иван Иванович', 'supplier', 'г. Подольск, ул. Промышленная, д. 5', 1),
('ООО "Торговый дом "Мука"', 'both', 'г. Санкт-Петербург, пр. Невский, д. 100', 1),
('ЗАО "Агро-Трейд"', 'supplier', 'г. Ростов-на-Дону, ул. Сельская, д. 20', 1),
('ООО "Пекарня "Свежий хлеб"', 'customer', 'г. Казань, ул. Пекарская, д. 15', 1))",

        R"(INSERT OR IGNORE INTO requisites (counterparty_id, inn, kpp, bik, bank_name, r_account, k_account) VALUES
(1, '7701234567', '770101001', '044525225', 'ПАО "Сбербанк"', '40702810100000001234', '30101810400000000225'),
(2, '501234567890', '501201001', '044525225', 'ПАО "Сбербанк"', '40802810100000005678', '30101810400000000225'),
(3, '7812345678', '781101001', '044030795', 'ПАО "Банк ВТБ"', '40702810100000009012', '30101810400000000795'),
(4, '6164567890', '616401001', '046015207', 'ПАО "Альфа-Банк"', '40702810100000004567', '30101810600000000207'),
(5, '1650123456', '165001001', '049205774', 'ПАО "Тинькофф Банк"', '40702810100000007890', '30101810145250000774'))",

        R"(INSERT OR IGNORE INTO documents (doc_type, number, date, status, sender_id, receiver_id, total_amount, notes, is_deleted) VALUES
('transfer', 'ТТН-001', date('now', '-2 days'), 'POSTED', 2, 1, 45500.00, 'ТТН на поставку муки', 0),
('transfer', 'ТТН-002', date('now', '-1 days'), 'DRAFT', 3, 1, 23500.00, 'ТТН на поставку круп', 0))",

        R"(INSERT OR IGNORE INTO document_lines (document_id, product_id, qty_kg, price, line_sum) VALUES
(1, 1, 1000.0, 45.50, 45500.00),
(2, 6, 500.0, 85.00, 42500.00),
(2, 7, 300.0, 65.00, 19500.00))"
    };

    for (const QString &statement : statements) {
        if (!executeQuery(statement, "loadTestDataInline")) {
            qWarning(migration) << "MigrationRunner: Failed to execute test data statement";
        }
    }

    return true;
}
