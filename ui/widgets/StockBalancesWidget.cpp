#include "StockBalancesWidget.h"
#include "DbManager.h"
#include "WriteOffForm.h"
#include "DecimalUtils.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QHeaderView>
#include <QDebug>
#include <QColor>
#include <QMessageBox>
#include <QDate>
#include <QDateTime>

// ---------------------
// Model
// ---------------------

StockBalancesModel::StockBalancesModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int StockBalancesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

int StockBalancesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 7; // ID | Товар | Активен | Остаток | Ед | Цена | Сумма
}

QVariant StockBalancesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size())
        return {};

    const BalanceItem &item = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return item.productId;
            case 1: return item.productName;
            case 2: return item.isActive ? "Да" : "Нет";
            case 3: return QString::number(item.balanceKg, 'f', 2);
            case 4: return item.unit;
            case 5: return decimalToString(item.price);
            case 6: return decimalToString(item.price * Decimal(item.balanceKg));
            default: return {};
        }
    }

    if (role == Qt::ForegroundRole) {
        if (item.isActive == 0)
            return QColor(Qt::gray);
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == 0) return QVariant(int(Qt::AlignCenter));
        if (index.column() == 2) return QVariant(int(Qt::AlignCenter));
        if (index.column() >= 3) return QVariant(int(Qt::AlignRight | Qt::AlignVCenter));
    }

    return {};
}

QVariant StockBalancesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return "ID";
            case 1: return "Товар";
            case 2: return "Активен";
            case 3: return "Остаток";
            case 4: return "Ед.";
            case 5: return "Цена";
            case 6: return "Сумма";
            default: return {};
        }
    }
    return {};
}

void StockBalancesModel::setShowInactive(bool v)
{
    if (m_showInactive == v) return;
    m_showInactive = v;
    refresh();
}

void StockBalancesModel::setHideZero(bool v)
{
    if (m_hideZero == v) return;
    m_hideZero = v;
    refresh();
}

void StockBalancesModel::refresh()
{
    beginResetModel();
    m_data.clear();

    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        endResetModel();
        return;
    }

    // ВАЖНО:
    // Баланс считаем только по НЕотмененным движениям (cancelled_flag = 0),
    // иначе в таблице будут "фантомные" остатки и можно списать в минус.
    QString sql = R"(
        SELECT
            p.id        AS id,
            p.name      AS name,
            p.is_active AS is_active,
            p.unit      AS unit,
            p.price     AS price,
            COALESCE(SUM(CASE WHEN im.cancelled_flag = 0 THEN im.qty_delta_kg ELSE 0 END), 0) AS balance
        FROM products p
        LEFT JOIN inventory_movements im ON p.id = im.product_id
        WHERE 1=1
    )";

    if (!m_showInactive) {
        sql += " AND p.is_active = 1 ";
    }

    sql += R"(
        GROUP BY p.id, p.name, p.is_active, p.unit, p.price
    )";

    if (m_hideZero) {
        sql += " HAVING ABS(balance) > 0.000001 ";
    }

    // sort убран -> сортируем по активности и имени
    sql += " ORDER BY p.is_active DESC, p.name ASC ";

    QSqlQuery query(db);
    if (!query.exec(sql)) {
        qWarning() << "StockBalancesModel::refresh SQL error:" << query.lastError().text();
        qWarning() << "SQL:" << sql;
        endResetModel();
        return;
    }

    while (query.next()) {
        BalanceItem item;
        item.productId = query.value("id").toInt();
        item.productName = query.value("name").toString();
        item.isActive = query.value("is_active").toInt();
        item.unit = query.value("unit").toString();
        item.price = decimalFromVariant(query.value("price"));
        item.balanceKg = query.value("balance").toDouble();
        m_data.append(item);
    }

    endResetModel();
}

// ---------------------
// Widget
// ---------------------

StockBalancesWidget::StockBalancesWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void StockBalancesWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Верхняя панель
    QHBoxLayout* topLayout = new QHBoxLayout();

    m_showInactiveCheck = new QCheckBox("Показывать неактивные", this);
    m_showInactiveCheck->setChecked(true);

    m_hideZeroCheck = new QCheckBox("Скрывать нулевые", this);
    m_hideZeroCheck->setChecked(false);

    topLayout->addWidget(m_showInactiveCheck);
    topLayout->addWidget(m_hideZeroCheck);
    topLayout->addStretch();

    m_writeOffButton = new QPushButton("Списать", this);
    topLayout->addWidget(m_writeOffButton);

    m_refreshButton = new QPushButton("Обновить", this);
    topLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(topLayout);

    m_tableView = new QTableView(this);
    m_model = new StockBalancesModel(this);

    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setAlternatingRowColors(true);

    m_tableView->setColumnWidth(0, 60);
    m_tableView->setColumnWidth(2, 80);
    m_tableView->setColumnWidth(3, 120);
    m_tableView->setColumnWidth(4, 60);
    m_tableView->setColumnWidth(5, 100);
    m_tableView->setColumnWidth(6, 120);

    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &StockBalancesWidget::onRefreshClicked);
    connect(m_writeOffButton, &QPushButton::clicked, this, &StockBalancesWidget::onWriteOffClicked);

    // Qt 6.9: stateChanged deprecated → checkStateChanged
    connect(m_showInactiveCheck, &QCheckBox::checkStateChanged, this, &StockBalancesWidget::onShowInactiveChanged);
    connect(m_hideZeroCheck, &QCheckBox::checkStateChanged, this, &StockBalancesWidget::onHideZeroChanged);

    m_model->refresh();
}

void StockBalancesWidget::onRefreshClicked()
{
    m_model->refresh();
}

void StockBalancesWidget::onShowInactiveChanged(Qt::CheckState state)
{
    m_model->setShowInactive(state == Qt::Checked);
}

void StockBalancesWidget::onHideZeroChanged(Qt::CheckState state)
{
    m_model->setHideZero(state == Qt::Checked);
}

int StockBalancesWidget::selectedProductId() const
{
    if (!m_tableView || !m_tableView->selectionModel())
        return 0;

    const QModelIndexList rows = m_tableView->selectionModel()->selectedRows();
    if (rows.isEmpty())
        return 0;

    const QModelIndex idx = rows.first();
    const QModelIndex idIdx = m_model->index(idx.row(), 0);
    return m_model->data(idIdx, Qt::DisplayRole).toInt();
}

void StockBalancesWidget::onWriteOffClicked()
{
    const int selectedPid = selectedProductId();
    if (selectedPid <= 0) {
        QMessageBox::warning(this, "Списание", "Выберите товар в таблице");
        return;
    }

    WriteOffForm dlg(this);
    dlg.setProductId(selectedPid);
    dlg.lockProductSelection(true);

    // Если выбранный товар не отобразился в форме (обычно: остаток 0 и он отфильтрован),
    // то диалог будет без товара/или ОК будет выключен — но лучше сказать сразу.
    if (dlg.productId() != selectedPid) {
        QMessageBox::warning(this, "Списание", "По выбранному товару нет остатка для списания.");
        return;
    }

    if (dlg.exec() != QDialog::Accepted)
        return;

    // ВАЖНО: списываем СТРОГО выбранный в таблице товар
    const int pid = selectedPid;
    const double qty = dlg.qtyKg();
    const QString reason = dlg.reason();

    if (qty <= 0.0) {
        QMessageBox::warning(this, "Списание", "Некорректное количество списания");
        return;
    }

    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return;
    }

    // Проверяем актуальный баланс по НЕотмененным движениям
    double balNow = 0.0;
    {
        QSqlQuery q(db);
        q.prepare("SELECT COALESCE(SUM(qty_delta_kg),0) "
                  "FROM inventory_movements WHERE product_id=:p AND cancelled_flag=0");
        q.bindValue(":p", pid);
        if (!q.exec() || !q.next()) {
            QMessageBox::critical(this, "Ошибка БД", "Не удалось получить остаток:\n" + q.lastError().text());
            return;
        }
        balNow = q.value(0).toDouble();
    }

    if (balNow + 1e-9 < qty) {
        QMessageBox::warning(this, "Списание",
                             QString("Недостаточно остатка для списания.\nДоступно: %1 кг")
                                 .arg(QString::number(balNow, 'f', 3)));
        return;
    }

    if (!db.transaction()) {
        QMessageBox::warning(this, "Ошибка БД", "Не удалось начать транзакцию:\n" + db.lastError().text());
        return;
    }

    int docId = 0;

    // 1) Создаём документ списания (doc_type = 'writeoff')
    {
        const QString number = QString("WRITEOFF-%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));

        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO documents (doc_type, number, date, status, sender_id, receiver_id, total_amount, notes)
            VALUES ('writeoff', :number, :date, 'POSTED', NULL, NULL, :total, :notes)
        )");
        q.bindValue(":number", number);
        q.bindValue(":date", QDate::currentDate().toString(Qt::ISODate));
        q.bindValue(":total", decimalToString(Decimal(0)));
        q.bindValue(":notes", reason.trimmed().isEmpty() ? QVariant() : QVariant(reason.trimmed()));

        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось создать документ списания:\n" + q.lastError().text());
            return;
        }
        docId = q.lastInsertId().toInt();
        if (docId <= 0) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось получить ID документа списания");
            return;
        }
    }

    // 2) Движение склада с отрицательным qty (расход)
    {
        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO inventory_movements (document_id, product_id, qty_delta_kg, movement_date, cancelled_flag)
            VALUES (:doc, :prod, :qty, :date, 0)
        )");
        q.bindValue(":doc", docId);
        q.bindValue(":prod", pid);
        q.bindValue(":qty", -qty);
        q.bindValue(":date", QDate::currentDate().toString(Qt::ISODate));

        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось создать движение списания:\n" + q.lastError().text());
            return;
        }
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::critical(this, "Ошибка БД", "Не удалось зафиксировать списание:\n" + db.lastError().text());
        return;
    }

    QMessageBox::information(this, "Списание", "Списание выполнено");
    m_model->refresh();
}
