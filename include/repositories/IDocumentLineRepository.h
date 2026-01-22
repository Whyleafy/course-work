#ifndef IDOCUMENTLINEREPOSITORY_H
#define IDOCUMENTLINEREPOSITORY_H

#include <QList>
#include <QString>

#include "DecimalUtils.h"

/**
 * @brief Структура данных строки документа
 */
struct DocumentLine {
    int id = 0;
    int documentId = 0;
    int productId = 0;
    double qtyKg = 0.0;
    Decimal price = 0;
    Decimal lineSum = 0;
    QString createdAt;
    
    bool isValid() const { return id > 0 && documentId > 0 && productId > 0; }
};

/**
 * @brief Интерфейс репозитория строк документов
 */
class IDocumentLineRepository
{
public:
    virtual ~IDocumentLineRepository() = default;
    
    /**
     * @brief Создать строку документа
     */
    virtual int create(const DocumentLine &line) = 0;
    
    /**
     * @brief Найти строку по ID
     */
    virtual DocumentLine findById(int id) = 0;
    
    /**
     * @brief Найти все строки документа
     */
    virtual QList<DocumentLine> findByDocument(int documentId) = 0;
    
    /**
     * @brief Удалить все строки документа
     */
    virtual bool deleteByDocument(int documentId) = 0;
    
    /**
     * @brief Обновить строку
     */
    virtual bool update(const DocumentLine &line) = 0;
};

#endif // IDOCUMENTLINEREPOSITORY_H
