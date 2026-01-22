#include "repositories/DocumentLineRepository.h"
#include "DecimalUtils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(docLineRepo, "repository.document_line")

DocumentLineRepository::DocumentLineRepository(QSqlDatabase db)
    : m_db(db)
{
    if (!m_db.isOpen()) {
        qCritical(docLineRepo) << "DocumentLineRepository: Database is not open";
    }
}

bool DocumentLineRepository::executeQuery(QSqlQuery& q, const QString& context) const
{
    if (!q.exec()) {
        qCritical(docLineRepo) << "DocumentLineRepository::" << context << "- SQL error:" << q.lastError().text();
        qCritical(docLineRepo) << "DocumentLineRepository::" << context << "- SQL:" << q.executedQuery();
        return false;
    }
    return true;
}

DocumentLine DocumentLineRepository::lineFromQuery(const QSqlQuery& q) const
{
    DocumentLine l;
    l.id = q.value("id").toInt();
    l.documentId = q.value("document_id").toInt();
    l.productId = q.value("product_id").toInt();
    l.qtyKg = q.value("qty_kg").toDouble();
    l.price = decimalFromVariant(q.value("price"));
    l.lineSum = decimalFromVariant(q.value("line_sum"));
    l.createdAt = q.value("created_at").toString();
    return l;
}

int DocumentLineRepository::create(const DocumentLine& line)
{
    if (line.documentId <= 0 || line.productId <= 0) return -1;

    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT INTO document_lines (document_id, product_id, qty_kg, price, line_sum)
        VALUES (:doc, :prod, :qty, :price, :sum)
    )");
    q.bindValue(":doc", line.documentId);
    q.bindValue(":prod", line.productId);
    q.bindValue(":qty", line.qtyKg);
    q.bindValue(":price", decimalToString(line.price));
    q.bindValue(":sum", decimalToString(line.lineSum));

    if (!executeQuery(q, "create")) return -1;

    const int id = q.lastInsertId().toInt();
    return id > 0 ? id : -1;
}

DocumentLine DocumentLineRepository::findById(int id)
{
    if (id <= 0) return DocumentLine();

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, document_id, product_id, qty_kg, price, line_sum, created_at
        FROM document_lines
        WHERE id = :id
    )");
    q.bindValue(":id", id);

    if (!executeQuery(q, "findById")) return DocumentLine();
    if (!q.next()) return DocumentLine();

    return lineFromQuery(q);
}

QList<DocumentLine> DocumentLineRepository::findByDocument(int documentId)
{
    QList<DocumentLine> res;
    if (documentId <= 0) return res;

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, document_id, product_id, qty_kg, price, line_sum, created_at
        FROM document_lines
        WHERE document_id = :doc
        ORDER BY id
    )");
    q.bindValue(":doc", documentId);

    if (!executeQuery(q, "findByDocument")) return res;

    while (q.next()) res.append(lineFromQuery(q));
    return res;
}

bool DocumentLineRepository::deleteByDocument(int documentId)
{
    if (documentId <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM document_lines WHERE document_id = :doc");
    q.bindValue(":doc", documentId);

    if (!executeQuery(q, "deleteByDocument")) return false;
    return true;
}

bool DocumentLineRepository::update(const DocumentLine& line)
{
    if (line.id <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare(R"(
        UPDATE document_lines
        SET product_id = :prod,
            qty_kg = :qty,
            price = :price,
            line_sum = :sum
        WHERE id = :id
    )");
    q.bindValue(":id", line.id);
    q.bindValue(":prod", line.productId);
    q.bindValue(":qty", line.qtyKg);
    q.bindValue(":price", decimalToString(line.price));
    q.bindValue(":sum", decimalToString(line.lineSum));

    if (!executeQuery(q, "update")) return false;
    return q.numRowsAffected() > 0;
}
