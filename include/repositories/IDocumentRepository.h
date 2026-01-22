#ifndef IDOCUMENTREPOSITORY_H
#define IDOCUMENTREPOSITORY_H

#include <QList>
#include <QString>
#include <QDate>

#include "DecimalUtils.h"

/**
 * @brief Типы документов
 */
enum class DocumentType {
    Supply,
    Sale,
    Return,
    Transfer
};

/**
 * @brief Статусы документов
 */
enum class DocumentStatus {
    Draft,
    Posted,
    Cancelled
};

/**
 * @brief Структура данных документа
 */
struct Document {
    int id = 0;
    DocumentType docType = DocumentType::Supply;
    QString number;
    QDate date = QDate::currentDate();
    DocumentStatus status = DocumentStatus::Draft;
    int senderId = 0;
    int receiverId = 0;
    Decimal totalAmount = 0;
    QString notes;
    QString createdAt;
    QString updatedAt;
    
    bool isValid() const { return id > 0 && !number.isEmpty(); }
    
    QString docTypeString() const;
    QString statusString() const;
    static DocumentType docTypeFromString(const QString &str);
    static DocumentStatus statusFromString(const QString &str);
};

/**
 * @brief Интерфейс репозитория документов
 */
class IDocumentRepository
{
public:
    virtual ~IDocumentRepository() = default;
    
    virtual int create(const Document &document) = 0;
    virtual Document findById(int id) = 0;
    virtual Document findByNumber(const QString &number, DocumentType type) = 0;
    virtual QList<Document> findAll() = 0;
    virtual QList<Document> findByStatus(DocumentStatus status) = 0;
    virtual QList<Document> findByDateRange(const QDate &from, const QDate &to) = 0;
    virtual bool update(const Document &document) = 0;
    virtual bool cancel(int id) = 0;
    virtual bool exists(int id) = 0;
};

#endif // IDOCUMENTREPOSITORY_H
