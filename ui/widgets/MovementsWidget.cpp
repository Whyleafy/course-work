#include "MovementsWidget.h"
#include "DbManager.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QHeaderView>
#include <QDebug>
#include <QColor>


MovementsModel::MovementsModel(QObject* parent)
    : QSqlQueryModel(parent)
{
}

QVariant MovementsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return {};


    if (role == Qt::TextAlignmentRole) {
        if (index.column() == 0) return QVariant(int(Qt::AlignCenter));
        if (index.column() == 7) return QVariant(int(Qt::AlignCenter));
        if (index.column() == 6) return QVariant(int(Qt::AlignRight | Qt::AlignVCenter));
    }

    if (role == Qt::ForegroundRole) {
        const int cancelled = QSqlQueryModel::data(this->index(index.row(), 7), Qt::DisplayRole).toInt();

        if (cancelled == 1) return QColor(Qt::gray);
    }

    if (role == Qt::DisplayRole && index.column() == 3) {
        const QString t = QSqlQueryModel::data(index, role).toString();
        if (t == "supply") return "Поставка";
        if (t == "sale") return "Продажа";
        if (t == "return") return "Возврат";
        if (t == "transfer") return "ТТН";
        return t;
    }

    if (role == Qt::DisplayRole && index.column() == 7) {
        const int v = QSqlQueryModel::data(index, role).toInt();
        return v == 1 ? "Да" : "Нет";
    }

    return QSqlQueryModel::data(index, role);
}

void MovementsModel::setOnlyPosted(bool v)
{
    if (m_onlyPosted == v) return;
    m_onlyPosted = v;
    refresh();
}

void MovementsModel::setShowStorno(bool v)
{
    if (m_showStorno == v) return;
    m_showStorno = v;
    refresh();
}

void MovementsModel::refresh()
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        setQuery(QSqlQuery()); // пусто
        return;
    }

    QString sql = R"(
        SELECT
            im.id                AS id,
            im.movement_date     AS date,
            d.number             AS doc_number,
            d.doc_type           AS doc_type,
            d.status             AS status,
            p.name               AS product_name,
            im.qty_delta_kg      AS qty_delta_kg,
            im.cancelled_flag    AS cancelled_flag
        FROM inventory_movements im
        LEFT JOIN documents d ON d.id = im.document_id
        LEFT JOIN products  p ON p.id = im.product_id
        WHERE 1=1
    )";

    if (m_onlyPosted) {
        sql += " AND d.status = 'POSTED' ";
    }

    if (!m_showStorno) {
        sql += " AND im.cancelled_flag = 0 ";
    }

    sql += " ORDER BY im.movement_date DESC, im.id DESC ";

    setQuery(sql, db);

    if (lastError().isValid()) {
        qWarning() << "MovementsModel SQL error:" << lastError().text();
        qWarning() << "SQL:" << sql;
    }

    setHeaderData(0, Qt::Horizontal, "ID");
    setHeaderData(1, Qt::Horizontal, "Дата");
    setHeaderData(2, Qt::Horizontal, "Документ");
    setHeaderData(3, Qt::Horizontal, "Тип");
    setHeaderData(4, Qt::Horizontal, "Статус");
    setHeaderData(5, Qt::Horizontal, "Товар");
    setHeaderData(6, Qt::Horizontal, "Δ (кг)");
    setHeaderData(7, Qt::Horizontal, "Storno");
}


MovementsWidget::MovementsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void MovementsWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* topLayout = new QHBoxLayout();

    m_onlyPostedCheck = new QCheckBox("Только проведенные", this);
    m_onlyPostedCheck->setChecked(false);

    m_showStornoCheck = new QCheckBox("Показывать storno", this);
    m_showStornoCheck->setChecked(true);

    topLayout->addWidget(m_onlyPostedCheck);
    topLayout->addWidget(m_showStornoCheck);
    topLayout->addStretch();

    m_refreshButton = new QPushButton("Обновить", this);
    topLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(topLayout);

    m_tableView = new QTableView(this);
    m_model = new MovementsModel(this);

    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setAlternatingRowColors(true);

    m_tableView->setColumnWidth(0, 60);
    m_tableView->setColumnWidth(1, 110);
    m_tableView->setColumnWidth(2, 110);
    m_tableView->setColumnWidth(3, 110);
    m_tableView->setColumnWidth(4, 110);
    m_tableView->setColumnWidth(5, 260);
    m_tableView->setColumnWidth(6, 90);
    m_tableView->setColumnWidth(7, 80);

    mainLayout->addWidget(m_tableView);

    connect(m_refreshButton, &QPushButton::clicked, this, &MovementsWidget::onRefreshClicked);

    connect(m_onlyPostedCheck, &QCheckBox::checkStateChanged, this, &MovementsWidget::onOnlyPostedChanged);
    connect(m_showStornoCheck, &QCheckBox::checkStateChanged, this, &MovementsWidget::onShowStornoChanged);

    m_model->refresh();
}

void MovementsWidget::onRefreshClicked()
{
    m_model->refresh();
}

void MovementsWidget::onOnlyPostedChanged(Qt::CheckState state)
{
    m_model->setOnlyPosted(state == Qt::Checked);
}

void MovementsWidget::onShowStornoChanged(Qt::CheckState state)
{
    m_model->setShowStorno(state == Qt::Checked);
}
