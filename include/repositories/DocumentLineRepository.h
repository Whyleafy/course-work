#ifndef DOCUMENTLINEREPOSITORY_H
#define DOCUMENTLINEREPOSITORY_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include "repositories/IDocumentLineRepository.h"

class DocumentLineRepository : public IDocumentLineRepository
{
public:
    explicit DocumentLineRepository(QSqlDatabase db);

    int create(const DocumentLine& line) override;
    DocumentLine findById(int id) override;
    QList<DocumentLine> findByDocument(int documentId) override;
    bool deleteByDocument(int documentId) override;
    bool update(const DocumentLine& line) override;

private:
    DocumentLine lineFromQuery(const QSqlQuery& q) const;
    bool executeQuery(QSqlQuery& q, const QString& context) const;

private:
    QSqlDatabase m_db;
};

#endif // DOCUMENTLINEREPOSITORY_H
