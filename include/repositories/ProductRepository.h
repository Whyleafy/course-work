#ifndef PRODUCTREPOSITORY_H
#define PRODUCTREPOSITORY_H

#include "IProductRepository.h"
#include <QSqlDatabase>
#include <QObject>

class ProductRepository : public QObject, public IProductRepository
{
    Q_OBJECT

public:
    explicit ProductRepository(QSqlDatabase db, QObject *parent = nullptr);
    
    int create(const Product &product) override;
    Product findById(int id) override;
    QList<Product> findAll() override;
    QList<Product> findAllIncludingInactive() override;
    bool update(const Product &product) override;
    bool deactivate(int id) override;
    bool activate(int id) override;
    bool exists(int id) override;

private:
    QSqlDatabase m_db;
    
    Product productFromQuery(const QSqlQuery &query) const;
    
    bool executeQuery(QSqlQuery &query, const QString &context) const;
};

#endif // PRODUCTREPOSITORY_H

