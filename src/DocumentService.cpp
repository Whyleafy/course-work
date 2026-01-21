#include "DocumentService.h"
#include "repositories/IDocumentRepository.h"
#include "repositories/IDocumentLineRepository.h"
#include "repositories/IStockRepository.h"
#include "repositories/IProductRepository.h"
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(docService, "service.document")

DocumentService::DocumentService(
    IDocumentRepository* docRepo,
    IDocumentLineRepository* lineRepo,
    IStockRepository* stockRepo,
    IProductRepository* productRepo,
    QObject *parent
)
    : QObject(parent)
    , m_docRepo(docRepo)
    , m_lineRepo(lineRepo)
    , m_stockRepo(stockRepo)
    , m_productRepo(productRepo)
{
}


bool DocumentService::postDocument(int documentId)
{
    Document doc = m_docRepo->findById(documentId);
    if (!doc.isValid()) {
        qWarning(docService) << "DocumentService::postDocument: Invalid document id" << documentId;
        return false;
    }
    
    if (doc.status != DocumentStatus::Draft) {
        qWarning(docService) << "DocumentService::postDocument: Document already posted or cancelled";
        return false;
    }
    
    QList<DocumentLine> lines = m_lineRepo->findByDocument(documentId);

    if (doc.docType == DocumentType::Transfer || doc.docType == DocumentType::Sale) {
        for (const auto &line : lines) {
            double balance = m_stockRepo->getStockBalance(line.productId);
            if (balance < line.qtyKg) {
                qWarning(docService) << "DocumentService::postDocument: Insufficient stock for product" << line.productId;
                return false;
            }
        }
    }

    double multiplier = (doc.docType == DocumentType::Supply || doc.docType == DocumentType::Return) ? 1.0 : -1.0;
    
    for (const auto &line : lines) {
        InventoryMovement movement;
        movement.documentId = documentId;
        movement.productId = line.productId;
        movement.qtyDeltaKg = line.qtyKg * multiplier;
        movement.movementDate = doc.date;
        movement.cancelledFlag = false;
        
        m_stockRepo->createMovement(movement);
    }

    doc.status = DocumentStatus::Posted;
    if (!m_docRepo->update(doc)) {
        qCritical(docService) << "DocumentService::postDocument: Failed to update document status";
        return false;
    }
    
    qInfo(docService) << "DocumentService::postDocument: Posted document" << documentId;
    return true;
}

bool DocumentService::cancelDocument(int documentId)
{
    Document doc = m_docRepo->findById(documentId);
    if (!doc.isValid()) {
        qWarning(docService) << "DocumentService::cancelDocument: Invalid document id" << documentId;
        return false;
    }
    
    if (doc.status == DocumentStatus::Cancelled) {
        qWarning(docService) << "DocumentService::cancelDocument: Document already cancelled";
        return false;
    }
    
    if (doc.status != DocumentStatus::Posted) {
        qWarning(docService) << "DocumentService::cancelDocument: Can only cancel posted documents";
        return false;
    }

    const QList<InventoryMovement> movements = m_stockRepo->findMovementsByDocument(documentId);
    bool cancelledAny = false;
    for (const auto &movement : movements) {
        if (movement.cancelledFlag) continue;
        cancelledAny = m_stockRepo->cancelMovement(movement.id) || cancelledAny;
    }

    if (!cancelledAny) {
        qWarning(docService) << "DocumentService::cancelDocument: No active movements to cancel";
        return false;
    }
    
    // Обновляем статус документа
    doc.status = DocumentStatus::Cancelled;
    if (!m_docRepo->cancel(documentId)) {
        qCritical(docService) << "DocumentService::cancelDocument: Failed to update document status";
        return false;
    }
    
    qInfo(docService) << "DocumentService::cancelDocument: Cancelled document" << documentId;
    return true;
}

