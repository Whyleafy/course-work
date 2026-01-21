#ifndef ISTOCKREPOSITORY_H
#define ISTOCKREPOSITORY_H

#include <QList>
#include <QDate>

struct InventoryMovement {
    int id = 0;
    int documentId = 0;
    int productId = 0;
    double qtyDeltaKg = 0.0;
    QDate movementDate = QDate::currentDate();
    bool cancelledFlag = false;
    QString createdAt;
    
    bool isValid() const { return id > 0 && documentId > 0 && productId > 0; }
};

struct StockBalance {
    int productId = 0;
    QString productName;
    double balanceKg = 0.0;
    QString unit = "кг";
    
    bool isValid() const { return productId > 0; }
};

/**
 * @brief Интерфейс репозитория складских операций
 * 
 * Принципы:
 * - Движение товара ТОЛЬКО через inventory_movements
 * - Отмена через cancelled_flag (storno)
 */
class IStockRepository
{
public:
    virtual ~IStockRepository() = default;
    
    /**
     * @brief Создать движение товара
     * @param movement Данные движения
     * @return ID созданного движения или -1 при ошибке
     */
    virtual int createMovement(const InventoryMovement &movement) = 0;
    
    /**
     * @brief Найти движение по ID
     */
    virtual InventoryMovement findMovementById(int id) = 0;
    
    /**
     * @brief Найти все движения по документу
     */
    virtual QList<InventoryMovement> findMovementsByDocument(int documentId) = 0;
    
    /**
     * @brief Найти все движения по товару
     */
    virtual QList<InventoryMovement> findMovementsByProduct(int productId) = 0;
    
    /**
     * @brief Отменить движение (storno через cancelled_flag)
     * @param id ID движения
     * @return true если успешно
     */
    virtual bool cancelMovement(int id) = 0;
    
    /**
     * @brief Получить текущий остаток товара (сумма всех неотмененных движений)
     * @param productId ID товара
     * @return Остаток в кг
     */
    virtual double getStockBalance(int productId) = 0;
    
    /**
     * @brief Получить остатки всех товаров
     */
    virtual QList<StockBalance> getAllStockBalances() = 0;
    
    /**
     * @brief Получить остатки только активных товаров
     */
    virtual QList<StockBalance> getActiveStockBalances() = 0;
};

#endif // ISTOCKREPOSITORY_H

