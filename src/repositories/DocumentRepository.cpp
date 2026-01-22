#include "repositories/DocumentRepository.h"
#include "DecimalUtils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(docRepo, "repository.document")

DocumentRepository::DocumentRepository(QSqlDatabase db)
    : m_db(db)
{
    if (!m_db.isOpen()) {
        qCritical(docRepo) << "DocumentRepository: Database is not open";
    }
}

QString DocumentRepository::docTypeToDb(DocumentType t)
{
    switch (t) {
        case DocumentType::Supply:        return "supply";
        case DocumentType::Sale:          return "sale";
        case DocumentType::Return:        return "return";
        case DocumentType::Transfer:      return "transfer";
    }
    return "supply";
}

QString DocumentRepository::statusToDb(DocumentStatus s)
{
    switch (s) {
        case DocumentStatus::Draft:     return "DRAFT";
        case DocumentStatus::Posted:    return "POSTED";
        case DocumentStatus::Cancelled: return "CANCELLED";
    }
    return "DRAFT";
}

DocumentType DocumentRepository::docTypeFromDb(const QString& s)
{
    const QString v = s.trimmed().toLower();
    if (v == "supply") return DocumentType::Supply;
    if (v == "sale") return DocumentType::Sale;
    if (v == "return") return DocumentType::Return;
    if (v == "transfer") return DocumentType::Transfer;
    return DocumentType::Supply;
}

DocumentStatus DocumentRepository::statusFromDb(const QString& s)
{
    const QString v = s.trimmed().toUpper();
    if (v == "POSTED") return DocumentStatus::Posted;
    if (v == "CANCELLED") return DocumentStatus::Cancelled;
    return DocumentStatus::Draft;
}

bool DocumentRepository::executeQuery(QSqlQuery& q, const QString& context) const
{
    if (!q.exec()) {
        qCritical(docRepo) << "DocumentRepository::" << context << "- SQL error:" << q.lastError().text();
        qCritical(docRepo) << "DocumentRepository::" << context << "- SQL:" << q.executedQuery();
        return false;
    }
    return true;
}

Document DocumentRepository::documentFromQuery(const QSqlQuery& q) const
{
    Document d;
    d.id = q.value("id").toInt();
    d.docType = docTypeFromDb(q.value("doc_type").toString());
    d.number = q.value("number").toString();
    d.date = QDate::fromString(q.value("date").toString(), Qt::ISODate);
    d.status = statusFromDb(q.value("status").toString());
    d.senderId = q.value("sender_id").isNull() ? 0 : q.value("sender_id").toInt();
    d.receiverId = q.value("receiver_id").isNull() ? 0 : q.value("receiver_id").toInt();
    d.totalAmount = decimalFromVariant(q.value("total_amount"));
    d.notes = q.value("notes").toString();
    d.createdAt = q.value("created_at").toString();
    d.updatedAt = q.value("updated_at").toString();
    return d;
}

int DocumentRepository::create(const Document& document)
{
    if (document.number.trimmed().isEmpty()) {
        qWarning(docRepo) << "DocumentRepository::create: empty number";
        return -1;
    }

    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT INTO documents (doc_type, number, date, status, sender_id, receiver_id, total_amount, notes)
        VALUES (:type, :number, :date, :status, :sender, :receiver, :total, :notes)
    )");
    q.bindValue(":type", docTypeToDb(document.docType));
    q.bindValue(":number", document.number.trimmed());
    q.bindValue(":date", document.date.toString(Qt::ISODate));
    q.bindValue(":status", statusToDb(document.status));
    q.bindValue(":sender", document.senderId == 0 ? QVariant() : QVariant(document.senderId));
    q.bindValue(":receiver", document.receiverId == 0 ? QVariant() : QVariant(document.receiverId));
    q.bindValue(":total", decimalToString(document.totalAmount));
    q.bindValue(":notes", document.notes.trimmed().isEmpty() ? QVariant() : QVariant(document.notes.trimmed()));

    if (!executeQuery(q, "create")) return -1;

    const int id = q.lastInsertId().toInt();
    return id > 0 ? id : -1;
}

Document DocumentRepository::findById(int id)
{
    if (id <= 0) return Document();

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, doc_type, number, date, status, sender_id, receiver_id, total_amount, notes, created_at, updated_at
        FROM documents
        WHERE id = :id
    )");
    q.bindValue(":id", id);

    if (!executeQuery(q, "findById")) return Document();
    if (!q.next()) return Document();

    return documentFromQuery(q);
}

Document DocumentRepository::findByNumber(const QString& number, DocumentType type)
{
    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, doc_type, number, date, status, sender_id, receiver_id, total_amount, notes, created_at, updated_at
        FROM documents
        WHERE number = :number AND doc_type = :type
        LIMIT 1
    )");
    q.bindValue(":number", number.trimmed());
    q.bindValue(":type", docTypeToDb(type));

    if (!executeQuery(q, "findByNumber")) return Document();
    if (!q.next()) return Document();

    return documentFromQuery(q);
}

QList<Document> DocumentRepository::findAll()
{
    QList<Document> res;
    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, doc_type, number, date, status, sender_id, receiver_id, total_amount, notes, created_at, updated_at
        FROM documents
        ORDER BY date DESC, id DESC
    )");

    if (!executeQuery(q, "findAll")) return res;
    while (q.next()) res.append(documentFromQuery(q));
    return res;
}

QList<Document> DocumentRepository::findByStatus(DocumentStatus status)
{
    QList<Document> res;
    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, doc_type, number, date, status, sender_id, receiver_id, total_amount, notes, created_at, updated_at
        FROM documents
        WHERE status = :status
        ORDER BY date DESC, id DESC
    )");
    q.bindValue(":status", statusToDb(status));

    if (!executeQuery(q, "findByStatus")) return res;
    while (q.next()) res.append(documentFromQuery(q));
    return res;
}

QList<Document> DocumentRepository::findByDateRange(const QDate& from, const QDate& to)
{
    QList<Document> res;
    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, doc_type, number, date, status, sender_id, receiver_id, total_amount, notes, created_at, updated_at
        FROM documents
        WHERE date >= :from AND date <= :to
        ORDER BY date DESC, id DESC
    )");
    q.bindValue(":from", from.toString(Qt::ISODate));
    q.bindValue(":to", to.toString(Qt::ISODate));

    if (!executeQuery(q, "findByDateRange")) return res;
    while (q.next()) res.append(documentFromQuery(q));
    return res;
}

bool DocumentRepository::update(const Document& document)
{
    if (document.id <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare(R"(
        UPDATE documents
        SET doc_type = :type,
            number = :number,
            date = :date,
            status = :status,
            sender_id = :sender,
            receiver_id = :receiver,
            total_amount = :total,
            notes = :notes,
            updated_at = datetime('now')
        WHERE id = :id
    )");
    q.bindValue(":id", document.id);
    q.bindValue(":type", docTypeToDb(document.docType));
    q.bindValue(":number", document.number.trimmed());
    q.bindValue(":date", document.date.toString(Qt::ISODate));
    q.bindValue(":status", statusToDb(document.status));
    q.bindValue(":sender", document.senderId == 0 ? QVariant() : QVariant(document.senderId));
    q.bindValue(":receiver", document.receiverId == 0 ? QVariant() : QVariant(document.receiverId));
    q.bindValue(":total", decimalToString(document.totalAmount));
    q.bindValue(":notes", document.notes.trimmed().isEmpty() ? QVariant() : QVariant(document.notes.trimmed()));

    if (!executeQuery(q, "update")) return false;
    return q.numRowsAffected() > 0;
}

bool DocumentRepository::cancel(int id)
{
    if (id <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare(R"(
        UPDATE documents
        SET status = 'CANCELLED',
            updated_at = datetime('now')
        WHERE id = :id
    )");
    q.bindValue(":id", id);

    if (!executeQuery(q, "cancel")) return false;
    return q.numRowsAffected() > 0;
}

bool DocumentRepository::exists(int id)
{
    if (id <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM documents WHERE id = :id LIMIT 1");
    q.bindValue(":id", id);

    if (!executeQuery(q, "exists")) return false;
    return q.next();
}
