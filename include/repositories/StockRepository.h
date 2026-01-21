#ifndef STOCKREPOSITORY_H
#define STOCKREPOSITORY_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include "repositories/IStockRepository.h"

class StockRepository : public IStockRepository
{
public:
    explicit StockRepository(QSqlDatabase db);

    int createMovement(const InventoryMovement& movement) override;
    InventoryMovement findMovementById(int id) override;
    QList<InventoryMovement> findMovementsByDocument(int documentId) override;
    QList<InventoryMovement> findMovementsByProduct(int productId) override;
    bool cancelMovement(int id) override;

    double getStockBalance(int productId) override;
    QList<StockBalance> getAllStockBalances() override;
    QList<StockBalance> getActiveStockBalances() override;

private:
    InventoryMovement movementFromQuery(const QSqlQuery& q) const;
    bool executeQuery(QSqlQuery& q, const QString& context) const;

private:
    QSqlDatabase m_db;
};

#endif // STOCKREPOSITORY_H
