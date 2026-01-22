#include "ProductForm.h"
#include "DbManager.h"
#include "DecimalUtils.h"

#include <QMessageBox>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QRegularExpressionValidator>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

ProductForm::ProductForm(QWidget *parent)
    : QDialog(parent)
    , m_productId(0)
{
    setupUi();
}

void ProductForm::setupUi()
{
    setWindowTitle("Товар");
    setModal(true);
    resize(450, 260);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* group = new QGroupBox("Данные товара", this);
    QFormLayout* form = new QFormLayout(group);

    m_nameEdit = new QLineEdit(this);
    form->addRow("Наименование:", m_nameEdit);

    m_unitEdit = new QLineEdit(this);
    m_unitEdit->setText("кг");
    form->addRow("Ед. изм.:", m_unitEdit);

    m_priceEdit = new QLineEdit(this);
    m_priceEdit->setPlaceholderText("0.00");
    auto* priceValidator = new QRegularExpressionValidator(
        QRegularExpression(R"(^\d+(?:[.,]\d{0,2})?$)"),
        m_priceEdit
    );
    m_priceEdit->setValidator(priceValidator);
    form->addRow("Цена:", m_priceEdit);


    m_activeCheck = new QCheckBox("Активен", this);
    m_activeCheck->setChecked(true);
    form->addRow("", m_activeCheck);

    mainLayout->addWidget(group);

    QHBoxLayout* buttons = new QHBoxLayout();
    buttons->addStretch();

    m_saveButton = new QPushButton("Сохранить", this);
    m_cancelButton = new QPushButton("Отмена", this);
    buttons->addWidget(m_saveButton);
    buttons->addWidget(m_cancelButton);

    mainLayout->addLayout(buttons);

    connect(m_saveButton, &QPushButton::clicked, this, &ProductForm::onSaveClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &ProductForm::onCancelClicked);
}

void ProductForm::loadData(int productId)
{
    m_productId = productId;

    if (m_productId <= 0) {
        setWindowTitle("Добавить товар");
        m_nameEdit->clear();
        m_unitEdit->setText("кг");
        m_priceEdit->setText(decimalToString(Decimal(0)));
        m_activeCheck->setChecked(true);
        return;
    }

    setWindowTitle(QString("Редактировать товар (ID %1)").arg(m_productId));

    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return;
    }

    QSqlQuery q(db);
    q.prepare("SELECT name, unit, price, is_active FROM products WHERE id = :id");
    q.bindValue(":id", m_productId);

    if (!q.exec()) {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось загрузить товар:\n" + q.lastError().text());
        return;
    }
    if (!q.next()) {
        QMessageBox::warning(this, "Не найдено", "Товар не найден");
        return;
    }

    m_nameEdit->setText(q.value(0).toString());
    m_unitEdit->setText(q.value(1).toString());
    m_priceEdit->setText(decimalToString(decimalFromVariant(q.value(2))));
    m_activeCheck->setChecked(q.value(3).toInt() == 1);
}

bool ProductForm::validateForm()
{
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите наименование товара");
        return false;
    }

    const QString unit = m_unitEdit->text().trimmed();
    if (unit.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите единицу измерения");
        return false;
    }

    return true;
}

void ProductForm::onSaveClicked()
{
    if (!validateForm())
        return;

    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return;
    }

    const QString name = m_nameEdit->text().trimmed();
    const QString unit = m_unitEdit->text().trimmed();
    const Decimal price = decimalFromString(m_priceEdit->text());
    const int isActive = m_activeCheck->isChecked() ? 1 : 0;

    if (!db.transaction()) {
        QMessageBox::warning(this, "Ошибка БД", "Не удалось начать транзакцию:\n" + db.lastError().text());
        return;
    }

    if (m_productId <= 0) {
        QSqlQuery q(db);
        q.prepare(
            "INSERT INTO products (name, unit, price, is_active, created_at, updated_at) "
            "VALUES (:name, :unit, :price, :active, datetime('now'), datetime('now'))"
        );
        q.bindValue(":name", name);
        q.bindValue(":unit", unit);
        q.bindValue(":price", decimalToString(price));
        q.bindValue(":active", isActive);


        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось добавить товар:\n" + q.lastError().text());
            return;
        }

        QVariant newId = q.lastInsertId();
        if (newId.isValid())
            m_productId = newId.toInt();

    } else {
        QSqlQuery q(db);
        q.prepare(
            "UPDATE products SET "
            "name = :name, unit = :unit, price = :price, is_active = :active, "
            "updated_at = datetime('now') "
            "WHERE id = :id"
        );
        q.bindValue(":name", name);
        q.bindValue(":unit", unit);
        q.bindValue(":price", decimalToString(price));
        q.bindValue(":active", isActive);
        q.bindValue(":id", m_productId);


        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось обновить товар:\n" + q.lastError().text());
            return;
        }
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::critical(this, "Ошибка БД", "Не удалось сохранить изменения:\n" + db.lastError().text());
        return;
    }

    emit saved();
    accept();
}

void ProductForm::onCancelClicked()
{
    reject();
}
