#ifndef COUNTERPARTYFORM_H
#define COUNTERPARTYFORM_H

#include <QDialog>

class QLineEdit;
class QComboBox;
class QTextEdit;
class QPushButton;

class CounterpartyForm : public QDialog
{
    Q_OBJECT

public:
    explicit CounterpartyForm(QWidget *parent = nullptr);

    // counterpartyId = 0 => режим добавления
    void loadData(int counterpartyId = 0);

signals:
    void saved();

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUi();
    void setupValidators();
    bool validateForm();

    bool saveToDb();                 // insert/update + requisites
    bool insertCounterparty(int& newId);
    bool updateCounterparty();
    bool upsertRequisites(int counterpartyId);

    bool loadCounterparty(int id);
    bool loadRequisites(int id);

private:
    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QTextEdit* m_addressEdit = nullptr;

    QLineEdit* m_innEdit = nullptr;
    QLineEdit* m_kppEdit = nullptr;
    QLineEdit* m_bikEdit = nullptr;
    QLineEdit* m_bankNameEdit = nullptr;
    QLineEdit* m_rAccountEdit = nullptr;
    QLineEdit* m_kAccountEdit = nullptr;

    QPushButton* m_saveButton = nullptr;
    QPushButton* m_cancelButton = nullptr;

    int m_counterpartyId = 0;
};

#endif // COUNTERPARTYFORM_H
