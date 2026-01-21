#include "repositories/ProductRepository.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(productRepo, "repository.product")

ProductRepository::ProductRepository(QSqlDatabase db, QObject *parent)
    : QObject(parent)
    , m_db(db)
{
    if (!m_db.isOpen()) {
        qCritical(productRepo) << "ProductRepository: Database is not open";
    }
}

int ProductRepository::create(const Product &product)
{
    if (product.name.isEmpty()) {
        qWarning(productRepo) << "ProductRepository::create: Product name is empty";
        return -1;
    }
    
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO products (name, unit, price, sort, is_active)
        VALUES (:name, :unit, :price, :sort, :is_active)
    )");
    
    query.bindValue(":name", product.name);
    query.bindValue(":unit", product.unit);
    query.bindValue(":price", product.price);
    query.bindValue(":sort", product.sort);
    query.bindValue(":is_active", product.isActive ? 1 : 0);
    
    if (!executeQuery(query, "create")) {
        return -1;
    }
    
    int newId = query.lastInsertId().toInt();
    qInfo(productRepo) << "ProductRepository::create: Created product with id" << newId;
    return newId;
}

Product ProductRepository::findById(int id)
{
    if (id <= 0) {
        return Product();
    }
    
    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, unit, price, sort, is_active, created_at, updated_at "
                  "FROM products WHERE id = :id");
    query.bindValue(":id", id);
    
    if (!executeQuery(query, "findById")) {
        return Product();
    }
    
    if (!query.next()) {
        qDebug(productRepo) << "ProductRepository::findById: Product with id" << id << "not found";
        return Product();
    }
    
    return productFromQuery(query);
}

QList<Product> ProductRepository::findAll()
{
    QList<Product> products;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, unit, price, is_active, created_at, updated_at "
              "FROM products WHERE is_active = 1 "
              "ORDER BY name");
    
    if (!executeQuery(query, "findAll")) {
        return products;
    }
    
    while (query.next()) {
        products.append(productFromQuery(query));
    }
    
    qDebug(productRepo) << "ProductRepository::findAll: Found" << products.size() << "active products";
    return products;
}

QList<Product> ProductRepository::findAllIncludingInactive()
{
    QList<Product> products;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT id, name, unit, price, is_active, created_at, updated_at "
              "FROM products "
              "ORDER BY name");
    
    if (!executeQuery(query, "findAllIncludingInactive")) {
        return products;
    }
    
    while (query.next()) {
        products.append(productFromQuery(query));
    }
    
    qDebug(productRepo) << "ProductRepository::findAllIncludingInactive: Found" << products.size() << "products";
    return products;
}

bool ProductRepository::update(const Product &product)
{
    if (!product.isValid()) {
        qWarning(productRepo) << "ProductRepository::update: Invalid product (id:" << product.id << ")";
        return false;
    }
    
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE products
        SET name = :name,
            unit = :unit,
            price = :price,
            sort = :sort,
            is_active = :is_active,
            updated_at = datetime('now')
        WHERE id = :id
    )");
    
    query.bindValue(":id", product.id);
    query.bindValue(":name", product.name);
    query.bindValue(":unit", product.unit);
    query.bindValue(":price", product.price);
    query.bindValue(":sort", product.sort);
    query.bindValue(":is_active", product.isActive ? 1 : 0);
    
    if (!executeQuery(query, "update")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        qWarning(productRepo) << "ProductRepository::update: No rows affected for id" << product.id;
        return false;
    }
    
    qInfo(productRepo) << "ProductRepository::update: Updated product with id" << product.id;
    return true;
}

bool ProductRepository::deactivate(int id)
{
    if (id <= 0) {
        qWarning(productRepo) << "ProductRepository::deactivate: Invalid id" << id;
        return false;
    }
    
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE products
        SET is_active = 0,
            updated_at = datetime('now')
        WHERE id = :id
    )");
    
    query.bindValue(":id", id);
    
    if (!executeQuery(query, "deactivate")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        qWarning(productRepo) << "ProductRepository::deactivate: Product with id" << id << "not found";
        return false;
    }
    
    qInfo(productRepo) << "ProductRepository::deactivate: Deactivated product with id" << id;
    return true;
}

bool ProductRepository::activate(int id)
{
    if (id <= 0) {
        qWarning(productRepo) << "ProductRepository::activate: Invalid id" << id;
        return false;
    }
    
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE products
        SET is_active = 1,
            updated_at = datetime('now')
        WHERE id = :id
    )");
    
    query.bindValue(":id", id);
    
    if (!executeQuery(query, "activate")) {
        return false;
    }
    
    if (query.numRowsAffected() == 0) {
        qWarning(productRepo) << "ProductRepository::activate: Product with id" << id << "not found";
        return false;
    }
    
    qInfo(productRepo) << "ProductRepository::activate: Activated product with id" << id;
    return true;
}

bool ProductRepository::exists(int id)
{
    if (id <= 0) {
        return false;
    }
    
    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM products WHERE id = :id LIMIT 1");
    query.bindValue(":id", id);
    
    if (!executeQuery(query, "exists")) {
        return false;
    }
    
    return query.next();
}

Product ProductRepository::productFromQuery(const QSqlQuery &query) const
{
    Product product;
    
    product.id = query.value("id").toInt();
    product.name = query.value("name").toString();
    product.unit = query.value("unit").toString();
    product.price = query.value("price").toDouble();
    product.sort = query.value("sort").toInt();
    product.isActive = query.value("is_active").toInt() == 1;
    product.createdAt = query.value("created_at").toString();
    product.updatedAt = query.value("updated_at").toString();
    
    return product;
}

bool ProductRepository::executeQuery(QSqlQuery &query, const QString &context) const
{
    if (!query.exec()) {
        qCritical(productRepo) << "ProductRepository::" << context 
                                << "- SQL error:" << query.lastError().text();
        qCritical(productRepo) << "ProductRepository::" << context 
                                << "- SQL:" << query.executedQuery();
        return false;
    }
    
    return true;
}

