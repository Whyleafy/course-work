#include "ProductsWidget.h"
#include "DbManager.h"
#include "ProductForm.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QSqlRecord>

ProductsWidget::ProductsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void ProductsWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_tableView = new QTableView(this);
    m_model = new QSqlTableModel(this, DbManager::instance().database());
    m_model->setTable("products");
    m_model->setFilter("is_active = 1");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    m_model->setHeaderData(m_model->fieldIndex("name"), Qt::Horizontal, "Наименование");
    m_model->setHeaderData(m_model->fieldIndex("unit"), Qt::Horizontal, "Ед. изм.");
    m_model->setHeaderData(m_model->fieldIndex("price"), Qt::Horizontal, "Цена");

    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);

    const int idIdx = m_model->fieldIndex("id");
    if (idIdx >= 0) m_tableView->hideColumn(idIdx);

    const int activeIdx = m_model->fieldIndex("is_active");
    if (activeIdx >= 0) m_tableView->hideColumn(activeIdx);

    const int createdIdx = m_model->fieldIndex("created_at");
    if (createdIdx >= 0) m_tableView->hideColumn(createdIdx);

    const int updatedIdx = m_model->fieldIndex("updated_at");
    if (updatedIdx >= 0) m_tableView->hideColumn(updatedIdx);

    const int sortIdx = m_model->fieldIndex("sort");
    if (sortIdx >= 0) m_tableView->hideColumn(sortIdx);


    mainLayout->addWidget(m_tableView);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_addButton = new QPushButton("Добавить", this);
    m_editButton = new QPushButton("Редактировать", this);
    m_refreshButton = new QPushButton("Обновить", this);

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(buttonLayout);

    connect(m_addButton, &QPushButton::clicked, this, &ProductsWidget::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &ProductsWidget::onEditClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &ProductsWidget::onRefreshClicked);

    refreshModel();
}

void ProductsWidget::refreshModel()
{
    m_model->select();
}

void ProductsWidget::onAddClicked()
{
    ProductForm form(this);
    form.loadData(0);

    if (form.exec() == QDialog::Accepted) {
        refreshModel();
    }
}

void ProductsWidget::onEditClicked()
{
    QModelIndexList selection = m_tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Выберите товар для редактирования");
        return;
    }

    const int row = selection.first().row();
    const int id = m_model->record(row).value("id").toInt();

    ProductForm form(this);
    form.loadData(id);

    if (form.exec() == QDialog::Accepted) {
        refreshModel();
    }
}

void ProductsWidget::onRefreshClicked()
{
    refreshModel();
}
