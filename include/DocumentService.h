#ifndef DOCUMENTSERVICE_H
#define DOCUMENTSERVICE_H

#include <QObject>
#include <QString>
#include "repositories/IDocumentRepository.h"
#include "repositories/IDocumentLineRepository.h"
#include "repositories/IStockRepository.h"
#include "repositories/IProductRepository.h"

class DocumentService : public QObject
{
    Q_OBJECT

public:
    explicit DocumentService(
        IDocumentRepository* docRepo,
        IDocumentLineRepository* lineRepo,
        IStockRepository* stockRepo,
        IProductRepository* productRepo,
        QObject *parent = nullptr
    );
    
    /**
     * @brief Провести документ (создать движения товара)
     */
    bool postDocument(int documentId);
    
    /**
     * @brief Отменить документ (storno)
     */
    bool cancelDocument(int documentId);

private:
    IDocumentRepository* m_docRepo;
    IDocumentLineRepository* m_lineRepo;
    IStockRepository* m_stockRepo;
    IProductRepository* m_productRepo;
};

#endif // DOCUMENTSERVICE_H

