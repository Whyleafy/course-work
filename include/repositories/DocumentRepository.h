#ifndef DOCUMENTREPOSITORY_H
#define DOCUMENTREPOSITORY_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include "repositories/IDocumentRepository.h"

class DocumentRepository : public IDocumentRepository
{
public:
    explicit DocumentRepository(QSqlDatabase db);

    int create(const Document& document) override;
    Document findById(int id) override;
    Document findByNumber(const QString& number, DocumentType type) override;
    QList<Document> findAll() override;
    QList<Document> findByStatus(DocumentStatus status) override;
    QList<Document> findByDateRange(const QDate& from, const QDate& to) override;
    bool update(const Document& document) override;
    bool cancel(int id) override;
    bool exists(int id) override;

private:
    static QString docTypeToDb(DocumentType t);
    static QString statusToDb(DocumentStatus s);
    static DocumentType docTypeFromDb(const QString& s);
    static DocumentStatus statusFromDb(const QString& s);

    Document documentFromQuery(const QSqlQuery& q) const;
    bool executeQuery(QSqlQuery& q, const QString& context) const;

private:
    QSqlDatabase m_db;
};

#endif // DOCUMENTREPOSITORY_H
