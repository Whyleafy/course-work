#include "SupplyWidget.h"
#include "DbManager.h"
#include "SupplyForm.h"
#include "DocumentService.h"

#include <QSqlTableModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlRecord>

SupplyWidget::SupplyWidget(DocumentService* docService, QWidget* parent)
    : QWidget(parent)
    , m_docService(docService)
{
    setupUi();
}

void SupplyWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);

    m_tableView = new QTableView(this);
    m_model = new QSqlTableModel(this, DbManager::instance().database());
    m_model->setTable("documents");
    m_model->setFilter("doc_type = 'supply'");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    m_model->setHeaderData(m_model->fieldIndex("number"), Qt::Horizontal, "Номер");
    m_model->setHeaderData(m_model->fieldIndex("date"), Qt::Horizontal, "Дата");
    m_model->setHeaderData(m_model->fieldIndex("status"), Qt::Horizontal, "Статус");
    m_model->setHeaderData(m_model->fieldIndex("sender_id"), Qt::Horizontal, "Поставщик (ID)");
    m_model->setHeaderData(m_model->fieldIndex("total_amount"), Qt::Horizontal, "Сумма");

    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);

    mainLayout->addWidget(m_tableView);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_addButton = new QPushButton("Добавить", this);
    m_editButton = new QPushButton("Редактировать", this);
    m_postButton = new QPushButton("Провести", this);
    m_cancelButton = new QPushButton("Отменить", this);
    m_refreshButton = new QPushButton("Обновить", this);

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_postButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(buttonLayout);

    connect(m_addButton, &QPushButton::clicked, this, &SupplyWidget::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &SupplyWidget::onEditClicked);
    connect(m_postButton, &QPushButton::clicked, this, &SupplyWidget::onPostClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SupplyWidget::onCancelClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &SupplyWidget::onRefreshClicked);

    refreshModel();
}

void SupplyWidget::refreshModel()
{
    m_model->select();
}

int SupplyWidget::selectedDocId() const
{
    auto sel = m_tableView->selectionModel()->selectedRows();
    if (sel.isEmpty()) return 0;

    const int row = sel.first().row();
    return m_model->record(row).value("id").toInt();
}

void SupplyWidget::onAddClicked()
{
    SupplyForm form(m_docService, this);
    form.loadData(0);
    if (form.exec() == QDialog::Accepted) {
        refreshModel();
    } else {
        refreshModel();
    }
}

void SupplyWidget::onEditClicked()
{
    const int docId = selectedDocId();
    if (docId <= 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите документ для редактирования");
        return;
    }

    SupplyForm form(m_docService, this);
    form.loadData(docId);
    if (form.exec() == QDialog::Accepted) {
        refreshModel();
    } else {
        refreshModel();
    }
}

void SupplyWidget::onPostClicked()
{
    const int docId = selectedDocId();
    if (docId <= 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите документ для проведения");
        return;
    }

    if (!m_docService) {
        QMessageBox::critical(this, "Ошибка", "DocumentService не передан");
        return;
    }

    if (!m_docService->postDocument(docId)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось провести документ");
        return;
    }

    QMessageBox::information(this, "Успех", "Документ проведен. Остатки увеличены.");
    refreshModel();
}

void SupplyWidget::onCancelClicked()
{
    const int docId = selectedDocId();
    if (docId <= 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите документ для отмены");
        return;
    }

    if (!m_docService) {
        QMessageBox::critical(this, "Ошибка", "DocumentService не передан");
        return;
    }

    if (QMessageBox::question(this, "Подтвердите", "Отменить документ (storno)?") != QMessageBox::Yes)
        return;

    if (!m_docService->cancelDocument(docId)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось отменить документ");
        return;
    }

    QMessageBox::information(this, "Успех", "Документ отменен (storno сделано).");
    refreshModel();
}

void SupplyWidget::onRefreshClicked()
{
    refreshModel();
}
