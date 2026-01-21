#include "repositories/IDocumentRepository.h"
#include <QDebug>

QString Document::docTypeString() const
{
    switch (docType) {
        case DocumentType::Supply: return "supply";
        case DocumentType::Sale: return "sale";
        case DocumentType::Return: return "return";
        case DocumentType::Transfer: return "transfer";
        default: return "supply";
    }
}

QString Document::statusString() const
{
    switch (status) {
        case DocumentStatus::Draft: return "DRAFT";
        case DocumentStatus::Posted: return "POSTED";
        case DocumentStatus::Cancelled: return "CANCELLED";
        default: return "DRAFT";
    }
}

DocumentType Document::docTypeFromString(const QString &str)
{
    if (str == "supply") return DocumentType::Supply;
    if (str == "sale") return DocumentType::Sale;
    if (str == "return") return DocumentType::Return;
    if (str == "transfer") return DocumentType::Transfer;
    return DocumentType::Supply;
}

DocumentStatus Document::statusFromString(const QString &str)
{
    if (str == "DRAFT") return DocumentStatus::Draft;
    if (str == "POSTED") return DocumentStatus::Posted;
    if (str == "CANCELLED") return DocumentStatus::Cancelled;
    return DocumentStatus::Draft;
}

