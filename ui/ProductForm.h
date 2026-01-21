#ifndef PRODUCTFORM_H
#define PRODUCTFORM_H

#include <QDialog>

class QLineEdit;
class QDoubleSpinBox;
class QCheckBox;
class QPushButton;

class ProductForm : public QDialog
{
    Q_OBJECT

public:
    explicit ProductForm(QWidget *parent = nullptr);

    // id == 0 -> добавление, иначе редактирование
    void loadData(int productId = 0);

signals:
    void saved();

private slots:
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUi();
    bool validateForm();

    int m_productId;

    QLineEdit* m_nameEdit;
    QLineEdit* m_unitEdit;
    QDoubleSpinBox* m_priceSpin;
    QCheckBox* m_activeCheck;

    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;
};

#endif // PRODUCTFORM_H
