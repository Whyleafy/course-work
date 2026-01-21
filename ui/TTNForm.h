#ifndef TTNFORM_H
#define TTNFORM_H

#include <QDialog>

class QLineEdit;
class QDateEdit;
class QComboBox;
class QTextEdit;
class QTableWidget;
class QPushButton;
class QLabel;

class TtnForm : public QDialog
{
    Q_OBJECT

public:
    explicit TtnForm(QWidget *parent = nullptr);

    // docId <= 0 => создание, иначе редактирование
    void loadData(int docId = 0);

signals:
    void saved();

private slots:
    void onAddLine();
    void onRemoveLine();
    void onRecalcTotals();
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUi();
    bool validateForm();

    bool loadCounterparties();
    bool loadProducts();

    // работа с БД
    bool insertDocument(int& outDocId);
    bool updateDocument();
    bool saveLines(int docId);
    bool recalcAndSaveTotal(int docId);

    int currentSelectedDocId() const { return m_docId; }

private:
    int m_docId = 0;

    // header
    QLineEdit* m_numberEdit = nullptr;
    QDateEdit* m_dateEdit = nullptr;
    QComboBox* m_senderCombo = nullptr;
    QComboBox* m_receiverCombo = nullptr;
    QTextEdit* m_notesEdit = nullptr;

    // lines
    QTableWidget* m_linesTable = nullptr;
    QLabel* m_totalLabel = nullptr;

    // buttons
    QPushButton* m_addLineButton = nullptr;
    QPushButton* m_removeLineButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

#endif // TTNFORM_H
