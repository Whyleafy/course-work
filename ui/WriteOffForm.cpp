#include "WriteOffForm.h"
#include "DbManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

static QString fmtKg(double v)
{
    return QString::number(v, 'f', 3);
}

WriteOffForm::WriteOffForm(QWidget* parent)
    : QDialog(parent)
{
    setupUi();
    loadProducts();

    if (m_pendingProductId > 0) {
        setProductId(m_pendingProductId);
    }

    updateLimitsForCurrentProduct();
}

void WriteOffForm::setupUi()
{
    setWindowTitle("Списание товара");
    setModal(true);
    resize(520, 260);

    auto* root = new QVBoxLayout(this);

    auto* group = new QGroupBox("Параметры списания", this);
    auto* form = new QFormLayout(group);

    m_productCombo = new QComboBox(this);
    form->addRow("Товар:", m_productCombo);

    m_availableLabel = new QLabel("Доступно: 0.000 кг", this);
    form->addRow("Остаток:", m_availableLabel);

    m_qtySpin = new QDoubleSpinBox(this);
    m_qtySpin->setDecimals(3);
    m_qtySpin->setMinimum(0.001);
    m_qtySpin->setMaximum(0.001); // позже выставим по остатку
    form->addRow("Количество (кг):", m_qtySpin);

    m_reasonEdit = new QTextEdit(this);
    m_reasonEdit->setMaximumHeight(80);
    form->addRow("Причина:", m_reasonEdit);

    root->addWidget(group);

    auto* btns = new QHBoxLayout();
    btns->addStretch();
    m_okBtn = new QPushButton("Списать", this);
    m_cancelBtn = new QPushButton("Отмена", this);
    btns->addWidget(m_okBtn);
    btns->addWidget(m_cancelBtn);
    root->addLayout(btns);

    connect(m_productCombo, &QComboBox::currentIndexChanged, this, [this]() {
        updateLimitsForCurrentProduct();
    });

    connect(m_okBtn, &QPushButton::clicked, this, [this]() {
        if (m_productCombo->currentIndex() < 0) {
            QMessageBox::warning(this, "Ошибка", "Выберите товар");
            return;
        }

        if (m_currentAvailable <= 0.0) {
            QMessageBox::warning(this, "Ошибка", "По выбранному товару нет остатка для списания");
            return;
        }

        const double qty = m_qtySpin->value();
        if (qty <= 0.0) {
            QMessageBox::warning(this, "Ошибка", "Количество должно быть > 0");
            return;
        }

        if (qty > m_currentAvailable + 1e-9) {
            QMessageBox::warning(this, "Ошибка",
                                 QString("Нельзя списать больше остатка.\nДоступно: %1 кг")
                                 .arg(fmtKg(m_currentAvailable)));
            return;
        }

        accept();
    });

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void WriteOffForm::loadProducts()
{
    m_productCombo->clear();

    QSqlDatabase db = DbManager::instance().database();
    QSqlQuery q(db);

    const QString sql = R"(
        SELECT
            p.id,
            p.name,
            p.is_active,
            COALESCE(SUM(im.qty_delta_kg), 0) AS bal
        FROM products p
        LEFT JOIN inventory_movements im ON im.product_id = p.id
        GROUP BY p.id, p.name, p.is_active
        HAVING bal > 0.000001
        ORDER BY p.is_active DESC, p.name ASC
    )";

    if (!q.exec(sql)) {
        QMessageBox::warning(this, "Ошибка БД", "Не удалось загрузить товары:\n" + q.lastError().text());
        return;
    }

    while (q.next()) {
        const int id = q.value(0).toInt();
        const QString name = q.value(1).toString();
        const int isActive = q.value(2).toInt();
        const double bal = q.value(3).toDouble();

        QString title = name + QString(" — %1 кг").arg(fmtKg(bal));
        if (isActive != 1) title += " (неактивный)";

        m_productCombo->addItem(title, id);
        m_productCombo->setItemData(m_productCombo->count() - 1, bal, Qt::UserRole + 1);
    }

    if (m_productCombo->count() <= 0) {
        m_availableLabel->setText("Доступно: 0.000 кг");
        m_qtySpin->setMaximum(0.001);
        m_qtySpin->setValue(0.001);
        m_okBtn->setEnabled(false);
    } else {
        m_okBtn->setEnabled(true);
    }
}

void WriteOffForm::updateLimitsForCurrentProduct()
{
    if (!m_productCombo || m_productCombo->currentIndex() < 0) {
        m_currentAvailable = 0.0;
        m_availableLabel->setText("Доступно: 0.000 кг");
        m_qtySpin->setMaximum(0.001);
        m_qtySpin->setValue(0.001);
        return;
    }

    const double bal = m_productCombo->currentData(Qt::UserRole + 1).toDouble();
    m_currentAvailable = bal;

    m_availableLabel->setText(QString("Доступно: %1 кг").arg(fmtKg(bal)));

    // максимум = остаток, минимум оставляем 0.001
    const double max = (bal > 0.001) ? bal : 0.001;
    m_qtySpin->setMaximum(max);

    // если старое значение больше нового max — подрежем
    if (m_qtySpin->value() > max) {
        m_qtySpin->setValue(max);
    }

    // если остаток совсем маленький — всё равно не даём уйти в ноль/минус
    if (bal <= 0.000001) {
        m_okBtn->setEnabled(false);
    } else {
        m_okBtn->setEnabled(true);
        // при первом выборе удобно поставить “всё доступное”
        if (m_qtySpin->value() < 0.001) m_qtySpin->setValue(0.001);
    }
}

int WriteOffForm::productId() const
{
    return m_productCombo->currentData().toInt();
}

double WriteOffForm::qtyKg() const
{
    return m_qtySpin->value();
}

QString WriteOffForm::reason() const
{
    return m_reasonEdit->toPlainText().trimmed();
}

void WriteOffForm::setProductId(int productId)
{
    if (productId <= 0) return;

    // если список ещё не загружен — запомним
    if (!m_productCombo || m_productCombo->count() == 0) {
        m_pendingProductId = productId;
        return;
    }

    const int idx = m_productCombo->findData(productId);
    if (idx >= 0) {
        m_productCombo->setCurrentIndex(idx);
        m_pendingProductId = 0;
        updateLimitsForCurrentProduct();
    } else {
        // товара нет (скорее всего у него нет остатка > 0 и он не попал в список)
        m_pendingProductId = productId;
    }
}

void WriteOffForm::lockProductSelection(bool lock)
{
    if (!m_productCombo) return;
    m_productCombo->setEnabled(!lock);
}
