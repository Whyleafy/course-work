#ifndef MIGRATIONRUNNER_H
#define MIGRATIONRUNNER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class MigrationRunner : public QObject
{
    Q_OBJECT

public:
    explicit MigrationRunner(QSqlDatabase db, QObject *parent = nullptr);
    
    bool runMigrations();
    
    bool tableExists(const QString &tableName);

private:
    QSqlDatabase m_db;
    
    bool createAllTables();

    bool createProductsTable();
    bool createCounterpartiesTable();
    bool createRequisitesTable();
    bool createDocumentsTable();
    bool createDocumentLinesTable();
    bool createInventoryMovementsTable();
    
    bool createIndexes();
    
    bool executeQuery(const QString &sql, const QString &errorContext = "");
    
    bool loadTestData();
    
    bool loadTestDataInline();
};

#endif // MIGRATIONRUNNER_H

