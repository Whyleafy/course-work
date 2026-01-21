#include "CounterpartiesWidget.h"
#include "CounterpartyForm.h"
#include "DbManager.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlRecord>

CounterpartiesWidget::CounterpartiesWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void CounterpartiesWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_tableView = new QTableView(this);
    m_model = new QSqlTableModel(this, DbManager::instance().database());
    m_model->setTable("counterparties");
    m_model->setFilter("is_active = 1");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    
    m_model->setHeaderData(m_model->fieldIndex("name"), Qt::Horizontal, "Наименование");
    m_model->setHeaderData(m_model->fieldIndex("type"), Qt::Horizontal, "Тип");
    m_model->setHeaderData(m_model->fieldIndex("address"), Qt::Horizontal, "Адрес");
    
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    
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
    
    connect(m_addButton, &QPushButton::clicked, this, &CounterpartiesWidget::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &CounterpartiesWidget::onEditClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &CounterpartiesWidget::onRefreshClicked);
    
    refreshModel();
}

void CounterpartiesWidget::refreshModel()
{
    m_model->select();
}

void CounterpartiesWidget::onAddClicked()
{
    CounterpartyForm form(this);
    if (form.exec() == QDialog::Accepted) {
        refreshModel();
    }
}

void CounterpartiesWidget::onEditClicked()
{
    QModelIndexList selection = m_tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Выберите контрагента для редактирования");
        return;
    }
    
    int row = selection.first().row();
    int id = m_model->record(row).value("id").toInt();
    
    CounterpartyForm form(this);
    form.loadData(id);
    if (form.exec() == QDialog::Accepted) {
        refreshModel();
    }
}

void CounterpartiesWidget::onRefreshClicked()
{
    refreshModel();
}

