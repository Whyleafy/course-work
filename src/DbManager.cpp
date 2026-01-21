#include "DbManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QDirIterator>

DbManager& DbManager::instance()
{
    static DbManager instance;
    return instance;
}

DbManager::DbManager()
{
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir;
    if (!dir.exists(dataPath)) {
        dir.mkpath(dataPath);
    }
    m_databasePath = dataPath + "/wholesale_trade.db";
}

bool DbManager::initialize()
{
    const QString connName = "WholesaleTradeConnection";
    if (QSqlDatabase::contains(connName)) {
        m_db = QSqlDatabase::database(connName);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    }

    qInfo() << "DbManager: Available SQL drivers:" << QSqlDatabase::drivers();

    m_db.setDatabaseName(m_databasePath);

    if (!m_db.open()) {
        qCritical() << "DbManager: Cannot open database:" << m_db.lastError().text();
        qCritical() << "DbManager: Database path:" << m_databasePath;
        return false;
    }

    qInfo() << "DbManager: Database opened successfully at" << m_databasePath;

    if (!enableForeignKeys()) {
        qCritical() << "DbManager: Cannot enable foreign keys";
        return false;
    }

    debugPrintResourcesOnce();

    if (!ensureInitialized()) {
        qCritical() << "DbManager: ensureInitialized failed";
        return false;
    }

    return true;
}

bool DbManager::isOpen() const
{
    return m_db.isOpen();
}

QSqlDatabase DbManager::database() const
{
    return m_db;
}

QString DbManager::databasePath() const
{
    return m_databasePath;
}

void DbManager::close()
{
    if (m_db.isOpen()) {
        m_db.close();
        qInfo() << "DbManager: Database connection closed";
    }
}

bool DbManager::enableForeignKeys()
{
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qCritical() << "DbManager: Cannot enable foreign keys:" << query.lastError().text();
        return false;
    }

    if (query.exec("PRAGMA foreign_keys")) {
        if (query.next()) {
            const int fkEnabled = query.value(0).toInt();
            if (fkEnabled == 1) {
                qInfo() << "DbManager: Foreign keys enabled";
                return true;
            }
        }
    }

    qWarning() << "DbManager: Foreign keys check failed";
    return false;
}

bool DbManager::hasTable(const QString& tableName) const
{
    return m_db.tables().contains(tableName);
}

bool DbManager::tableHasAnyRows(const QString& tableName) const
{
    if (!hasTable(tableName)) return false;

    QSqlQuery q(m_db);
    if (!q.exec(QString("SELECT 1 FROM %1 LIMIT 1").arg(tableName))) {
        qWarning() << "DbManager: tableHasAnyRows failed for" << tableName << ":" << q.lastError().text();
        return false;
    }
    return q.next();
}

bool DbManager::ensureInitialized()
{
    const QString schemaPath = resolveSqlResourcePath("database_schema.sql");
    const QString testDataPath = resolveSqlResourcePath("test_data.sql");

    if (schemaPath.isEmpty()) {
        qCritical() << "DbManager: Cannot resolve database_schema.sql in resources";
        return false;
    }
    if (testDataPath.isEmpty()) {
        qCritical() << "DbManager: Cannot resolve test_data.sql in resources";
        return false;
    }

    if (!hasTable("products")) {
        qInfo() << "DbManager: Empty DB detected. Creating schema...";

        if (!execSqlFile(schemaPath)) {
            qCritical() << "DbManager: Schema execution failed:" << schemaPath;
            return false;
        }

        qInfo() << "DbManager: Schema created.";
    } else {
        qInfo() << "DbManager: Tables exist. (products found)";
    }

    if (!tableHasAnyRows("products")) {
        qInfo() << "DbManager: No rows in products. Inserting test data...";

        if (!execSqlFile(testDataPath)) {
            qCritical() << "DbManager: Test data execution failed:" << testDataPath;
            return false;
        }

        qInfo() << "DbManager: Test data inserted.";
    } else {
        qInfo() << "DbManager: Test data already present (products not empty).";
    }

    qInfo() << "DbManager: Current tables:" << m_db.tables();
    return true;
}

QString DbManager::resolveSqlResourcePath(const QString& fileName) const
{
    const QString preferred = QString(":/sql/%1").arg(fileName);
    if (QFile::exists(preferred))
        return preferred;

    QDirIterator it(":/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString p = it.next();
        if (p.endsWith("/" + fileName) || p.endsWith("\\" + fileName)) {
            if (QFile::exists(p)) return p;
        }
    }

    return QString();
}

void DbManager::debugPrintResourcesOnce() const
{
    static bool printed = false;
    if (printed) return;
    printed = true;

    const QString schemaPref = ":/sql/database_schema.sql";
    const QString testPref   = ":/sql/test_data.sql";

    qInfo() << "DbManager: exists" << schemaPref << QFile::exists(schemaPref);
    qInfo() << "DbManager: exists" << testPref   << QFile::exists(testPref);

    const QString schemaResolved = resolveSqlResourcePath("database_schema.sql");
    const QString testResolved   = resolveSqlResourcePath("test_data.sql");

    qInfo() << "DbManager: resolved schema path:" << schemaResolved;
    qInfo() << "DbManager: resolved test path:" << testResolved;

    qInfo() << "DbManager: resource dir :/sql entries:" << QDir(":/sql").entryList(QDir::Files);

    QStringList sqlFiles;
    QDirIterator it(":/", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString p = it.next();
        if (p.endsWith(".sql")) sqlFiles << p;
    }
    qInfo() << "DbManager: all .sql in resources:" << sqlFiles;
}

bool DbManager::execSqlFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "DbManager: Can't open SQL file:" << path << f.errorString();
        return false;
    }

    const QString raw = QString::fromUtf8(f.readAll());
    f.close();

    QString cleaned;
    cleaned.reserve(raw.size());
    const QStringList lines = raw.split('\n');
    for (const QString& line : lines) {
        const QString t = line.trimmed();
        if (t.startsWith("--")) continue;
        cleaned += line;
        cleaned += '\n';
    }

    const QStringList statements = cleaned.split(';', Qt::SkipEmptyParts);

    if (!m_db.transaction()) {
        qWarning() << "DbManager: transaction() failed:" << m_db.lastError().text();
    }

    for (QString stmt : statements) {
        stmt = stmt.trimmed();
        if (stmt.isEmpty()) continue;

        QSqlQuery q(m_db);
        if (!q.exec(stmt)) {
            qCritical() << "DbManager: SQL error:" << q.lastError().text();
            qCritical() << "DbManager: Statement:" << stmt;
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        qCritical() << "DbManager: commit() failed:" << m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    return true;
}
