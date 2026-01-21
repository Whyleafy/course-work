#include "repositories/StockRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(stockRepo, "repository.stock")

StockRepository::StockRepository(QSqlDatabase db)
    : m_db(db)
{
    if (!m_db.isOpen()) {
        qCritical(stockRepo) << "StockRepository: Database is not open";
    }
}

bool StockRepository::executeQuery(QSqlQuery& q, const QString& context) const
{
    if (!q.exec()) {
        qCritical(stockRepo) << "StockRepository::" << context << "- SQL error:" << q.lastError().text();
        qCritical(stockRepo) << "StockRepository::" << context << "- SQL:" << q.executedQuery();
        return false;
    }
    return true;
}

InventoryMovement StockRepository::movementFromQuery(const QSqlQuery& q) const
{
    InventoryMovement m;
    m.id = q.value("id").toInt();
    m.documentId = q.value("document_id").toInt();
    m.productId = q.value("product_id").toInt();
    m.qtyDeltaKg = q.value("qty_delta_kg").toDouble();
    m.movementDate = QDate::fromString(q.value("movement_date").toString(), Qt::ISODate);
    m.cancelledFlag = q.value("cancelled_flag").toInt() == 1;
    m.createdAt = q.value("created_at").toString();
    return m;
}

int StockRepository::createMovement(const InventoryMovement& movement)
{
    if (movement.documentId <= 0 || movement.productId <= 0) return -1;

    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT INTO inventory_movements (document_id, product_id, qty_delta_kg, movement_date, cancelled_flag)
        VALUES (:doc, :prod, :qty, :date, :cancelled)
    )");
    q.bindValue(":doc", movement.documentId);
    q.bindValue(":prod", movement.productId);
    q.bindValue(":qty", movement.qtyDeltaKg);
    q.bindValue(":date", movement.movementDate.toString(Qt::ISODate));
    q.bindValue(":cancelled", movement.cancelledFlag ? 1 : 0);

    if (!executeQuery(q, "createMovement")) return -1;

    const int id = q.lastInsertId().toInt();
    return id > 0 ? id : -1;
}

InventoryMovement StockRepository::findMovementById(int id)
{
    if (id <= 0) return InventoryMovement();

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, document_id, product_id, qty_delta_kg, movement_date, cancelled_flag, created_at
        FROM inventory_movements
        WHERE id = :id
    )");
    q.bindValue(":id", id);

    if (!executeQuery(q, "findMovementById")) return InventoryMovement();
    if (!q.next()) return InventoryMovement();

    return movementFromQuery(q);
}

QList<InventoryMovement> StockRepository::findMovementsByDocument(int documentId)
{
    QList<InventoryMovement> res;
    if (documentId <= 0) return res;

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, document_id, product_id, qty_delta_kg, movement_date, cancelled_flag, created_at
        FROM inventory_movements
        WHERE document_id = :doc
        ORDER BY id
    )");
    q.bindValue(":doc", documentId);

    if (!executeQuery(q, "findMovementsByDocument")) return res;
    while (q.next()) res.append(movementFromQuery(q));
    return res;
}

QList<InventoryMovement> StockRepository::findMovementsByProduct(int productId)
{
    QList<InventoryMovement> res;
    if (productId <= 0) return res;

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, document_id, product_id, qty_delta_kg, movement_date, cancelled_flag, created_at
        FROM inventory_movements
        WHERE product_id = :prod
        ORDER BY id
    )");
    q.bindValue(":prod", productId);

    if (!executeQuery(q, "findMovementsByProduct")) return res;
    while (q.next()) res.append(movementFromQuery(q));
    return res;
}

bool StockRepository::cancelMovement(int id)
{
    if (id <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare(R"(
        UPDATE inventory_movements
        SET cancelled_flag = 1
        WHERE id = :id
    )");
    q.bindValue(":id", id);

    if (!executeQuery(q, "cancelMovement")) return false;
    return q.numRowsAffected() > 0;
}

double StockRepository::getStockBalance(int productId)
{
    if (productId <= 0) return 0.0;

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT COALESCE(SUM(qty_delta_kg), 0) AS bal
        FROM inventory_movements
        WHERE product_id = :prod
          AND cancelled_flag = 0
    )");
    q.bindValue(":prod", productId);

    if (!executeQuery(q, "getStockBalance")) return 0.0;
    if (!q.next()) return 0.0;

    return q.value("bal").toDouble();
}

QList<StockBalance> StockRepository::getAllStockBalances()
{
    QList<StockBalance> res;

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT p.id AS product_id,
               p.name AS product_name,
               p.unit AS unit,
               COALESCE(SUM(im.qty_delta_kg), 0) AS balance_kg
        FROM products p
        LEFT JOIN inventory_movements im
               ON im.product_id = p.id
              AND im.cancelled_flag = 0
        GROUP BY p.id, p.name, p.unit
        ORDER BY p.name
    )");

    if (!executeQuery(q, "getAllStockBalances")) return res;

    while (q.next()) {
        StockBalance b;
        b.productId = q.value("product_id").toInt();
        b.productName = q.value("product_name").toString();
        b.unit = q.value("unit").toString();
        b.balanceKg = q.value("balance_kg").toDouble();
        res.append(b);
    }

    return res;
}

QList<StockBalance> StockRepository::getActiveStockBalances()
{
    QList<StockBalance> res;

    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT p.id AS product_id,
               p.name AS product_name,
               p.unit AS unit,
               COALESCE(SUM(im.qty_delta_kg), 0) AS balance_kg
        FROM products p
        LEFT JOIN inventory_movements im
               ON im.product_id = p.id
              AND im.cancelled_flag = 0
        WHERE p.is_active = 1
        GROUP BY p.id, p.name, p.unit
        ORDER BY p.name
    )");

    if (!executeQuery(q, "getActiveStockBalances")) return res;

    while (q.next()) {
        StockBalance b;
        b.productId = q.value("product_id").toInt();
        b.productName = q.value("product_name").toString();
        b.unit = q.value("unit").toString();
        b.balanceKg = q.value("balance_kg").toDouble();
        res.append(b);
    }

    return res;
}
