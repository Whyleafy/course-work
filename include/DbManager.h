#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class DbManager : public QObject
{
    Q_OBJECT

public:
    static DbManager& instance();

    bool initialize();
    bool isOpen() const;

    QSqlDatabase database() const;
    QString databasePath() const;

    void close();

private:
    explicit DbManager();
    ~DbManager() override = default;

    DbManager(const DbManager&) = delete;
    DbManager& operator=(const DbManager&) = delete;

private:
    QSqlDatabase m_db;
    QString m_databasePath;

private:
    bool enableForeignKeys();

    bool ensureInitialized();
    bool hasTable(const QString& tableName) const;
    bool tableHasAnyRows(const QString& tableName) const;

    bool execSqlFile(const QString& path);

    QString resolveSqlResourcePath(const QString& fileName) const;
    void debugPrintResourcesOnce() const;
};

#endif // DBMANAGER_H
