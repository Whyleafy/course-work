#ifndef WRITEOFFFORM_H
#define WRITEOFFFORM_H

#include <QDialog>

class QComboBox;
class QDoubleSpinBox;
class QTextEdit;
class QPushButton;
class QLabel;

class WriteOffForm : public QDialog
{
    Q_OBJECT
public:
    explicit WriteOffForm(QWidget* parent = nullptr);

    int productId() const;
    double qtyKg() const;
    QString reason() const;

    // можно заранее установить товар (например, из остатков)
    void setProductId(int productId);

    // если вызвали из остатков — можно запретить менять товар
    void lockProductSelection(bool lock);

private:
    void setupUi();
    void loadProducts();
    void updateLimitsForCurrentProduct(); // NEW

private:
    QComboBox* m_productCombo = nullptr;
    QDoubleSpinBox* m_qtySpin = nullptr;
    QTextEdit* m_reasonEdit = nullptr;

    QLabel* m_availableLabel = nullptr;  // NEW

    QPushButton* m_okBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    int m_pendingProductId = 0;          // NEW
    double m_currentAvailable = 0.0;     // NEW
};

#endif // WRITEOFFFORM_H
