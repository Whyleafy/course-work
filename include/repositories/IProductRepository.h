#ifndef IPRODUCTREPOSITORY_H
#define IPRODUCTREPOSITORY_H

#include <QList>
#include <QVariant>
#include <QString>

struct Product {
    int id = 0;
    QString name;
    QString unit = "кг";
    double price = 0.0;
    int sort = 0;
    bool isActive = true;
    QString createdAt;
    QString updatedAt;
    
    bool isValid() const { return id > 0 && !name.isEmpty(); }
};

class IProductRepository
{
public:
    virtual ~IProductRepository() = default;
    
    virtual int create(const Product &product) = 0;
    
    virtual Product findById(int id) = 0;

    virtual QList<Product> findAll() = 0;

    virtual QList<Product> findAllIncludingInactive() = 0;

    virtual bool update(const Product &product) = 0;

    virtual bool deactivate(int id) = 0;

    virtual bool activate(int id) = 0;

    virtual bool exists(int id) = 0;
};

#endif // IPRODUCTREPOSITORY_H

