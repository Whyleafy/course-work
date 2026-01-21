#ifndef SUPPLYFORM_H
#define SUPPLYFORM_H

#include <QDialog>
#include <QList>
#include <QString>

class QLineEdit;
class QDateEdit;
class QComboBox;
class QTableWidget;
class QLabel;
class QPushButton;

class DocumentService;

class SupplyForm : public QDialog
{
    Q_OBJECT
public:
    explicit SupplyForm(DocumentService* docService, QWidget* parent = nullptr);

    void loadData(int documentId); // если 0 — режим "Добавить"

signals:
    void saved();

private slots:
    void onAddLineClicked();
    void onRemoveLineClicked();
    void onSaveClicked();
    void onPostClicked();
    void onCancelDocumentClicked();
    void onLineChanged();

private:
    void setupUi();
    void reloadProducts();
    void reloadCounterparties();
    void setUiReadOnly(bool ro);

    bool validateForm();
    void recalcTotals();

    bool saveToDb();
    bool createDocument();
    bool updateDocument();

    bool deleteLines(int documentId);
    bool insertLines(int documentId);

    int currentSelectedProductId(int row) const;
    double currentQty(int row) const;
    double productPriceById(int productId) const;
    QString productNameById(int productId) const;

private:
    DocumentService* m_docService = nullptr;

    int m_documentId = 0;
    QString m_status = "DRAFT";

    struct ProductItem {
        int id = 0;
        QString name;
        double price = 0.0;
        QString unit = "кг";
    };
    QList<ProductItem> m_products;

    QLineEdit*   m_numberEdit = nullptr;
    QDateEdit*   m_dateEdit = nullptr;
    QComboBox*   m_senderCombo = nullptr;

    QTableWidget* m_linesTable = nullptr;
    QLabel*       m_totalLabel = nullptr;

    QPushButton*  m_addLineBtn = nullptr;
    QPushButton*  m_removeLineBtn = nullptr;

    QPushButton*  m_saveBtn = nullptr;
    QPushButton*  m_postBtn = nullptr;
    QPushButton*  m_cancelBtn = nullptr;
    QPushButton*  m_closeBtn = nullptr;
};

#endif // SUPPLYFORM_H
