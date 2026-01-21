#include "TTNWidget.h"
#include "DbManager.h"
#include "TTNForm.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

TTNWidget::TTNWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void TTNWidget::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_tableView = new QTableView(this);
    m_model = new QSqlTableModel(this, DbManager::instance().database());
    m_model->setTable("documents");

    m_model->setFilter("doc_type = 'transfer' AND is_deleted = 0");

    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);

    m_model->setHeaderData(m_model->fieldIndex("number"), Qt::Horizontal, "Номер");
    m_model->setHeaderData(m_model->fieldIndex("date"), Qt::Horizontal, "Дата");
    m_model->setHeaderData(m_model->fieldIndex("status"), Qt::Horizontal, "Статус");
    m_model->setHeaderData(m_model->fieldIndex("total_amount"), Qt::Horizontal, "Сумма");

    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setStretchLastSection(true);

    const int idIdx = m_model->fieldIndex("id");
    if (idIdx >= 0) m_tableView->hideColumn(idIdx);

    const int delIdx = m_model->fieldIndex("is_deleted");
    if (delIdx >= 0) m_tableView->hideColumn(delIdx);

    mainLayout->addWidget(m_tableView);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_addButton = new QPushButton("Добавить", this);
    m_editButton = new QPushButton("Редактировать", this);
    m_postButton = new QPushButton("Провести", this);
    m_cancelButton = new QPushButton("Отменить", this);
    m_deleteButton = new QPushButton("Удалить", this);
    m_refreshButton = new QPushButton("Обновить", this);

    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_postButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(buttonLayout);

    connect(m_addButton, &QPushButton::clicked, this, &TTNWidget::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &TTNWidget::onEditClicked);
    connect(m_postButton, &QPushButton::clicked, this, &TTNWidget::onPostClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &TTNWidget::onCancelClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &TTNWidget::onDeleteClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &TTNWidget::onRefreshClicked);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TTNWidget::onSelectionChanged);

    refreshModel();
    updateButtonsByStatus();
}

void TTNWidget::refreshModel()
{
    m_model->select();
}

int TTNWidget::selectedDocId() const
{
    QModelIndexList selection = m_tableView->selectionModel()->selectedRows();
    if (selection.isEmpty())
        return 0;

    int row = selection.first().row();
    return m_model->record(row).value("id").toInt();
}

QString TTNWidget::selectedDocStatus() const
{
    QModelIndexList selection = m_tableView->selectionModel()->selectedRows();
    if (selection.isEmpty())
        return QString();

    int row = selection.first().row();
    return m_model->record(row).value("status").toString();
}

void TTNWidget::onSelectionChanged()
{
    updateButtonsByStatus();
}

void TTNWidget::updateButtonsByStatus()
{
    const int docId = selectedDocId();
    if (docId <= 0) {
        m_editButton->setEnabled(false);
        m_postButton->setEnabled(false);
        m_cancelButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        return;
    }

    const QString status = selectedDocStatus();

    if (status == "DRAFT") {
        m_editButton->setEnabled(true);
        m_postButton->setEnabled(true);
        m_cancelButton->setEnabled(false);
        m_deleteButton->setEnabled(true);   // физическое удаление
        m_deleteButton->setText("Удалить");
    } else if (status == "POSTED") {
        m_editButton->setEnabled(false);
        m_postButton->setEnabled(false);
        m_cancelButton->setEnabled(true);
        m_deleteButton->setEnabled(true);   // “удалить” = отменить
        m_deleteButton->setText("Удалить");
    } else if (status == "CANCELLED") {
        m_editButton->setEnabled(false);
        m_postButton->setEnabled(false);
        m_cancelButton->setEnabled(false);

        // IMPORTANT: теперь разрешаем "удалять" отменённые — это soft-delete (скрыть)
        m_deleteButton->setEnabled(true);
        m_deleteButton->setText("Скрыть");
    } else {
        m_editButton->setEnabled(false);
        m_postButton->setEnabled(false);
        m_cancelButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        m_deleteButton->setText("Удалить");
    }
}

void TTNWidget::onAddClicked()
{
    TtnForm form(this);
    form.loadData(0);
    if (form.exec() == QDialog::Accepted) {
        refreshModel();
        updateButtonsByStatus();
    }
}

void TTNWidget::onEditClicked()
{
    const int docId = selectedDocId();
    if (docId <= 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите ТТН для редактирования");
        return;
    }

    const QString status = selectedDocStatus();
    if (status != "DRAFT") {
        QMessageBox::warning(this, "Предупреждение", "Редактировать можно только черновик (DRAFT)");
        return;
    }

    TtnForm form(this);
    form.loadData(docId);
    if (form.exec() == QDialog::Accepted) {
        refreshModel();
        updateButtonsByStatus();
    }
}

void TTNWidget::onPostClicked()
{
    const int docId = selectedDocId();
    if (docId <= 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите ТТН для проведения");
        return;
    }

    const QString status = selectedDocStatus();
    if (status != "DRAFT") {
        QMessageBox::warning(this, "Предупреждение", "Провести можно только документ в статусе DRAFT");
        return;
    }

    if (!postDocumentSql(docId)) {
        return;
    }

    QMessageBox::information(this, "Успех", "ТТН проведена");
    refreshModel();
    updateButtonsByStatus();
}

void TTNWidget::onCancelClicked()
{
    const int docId = selectedDocId();
    if (docId <= 0) {
        QMessageBox::warning(this, "Предупреждение", "Выберите ТТН для отмены");
        return;
    }

    const QString status = selectedDocStatus();
    if (status != "POSTED") {
        QMessageBox::warning(this, "Предупреждение", "Отменить можно только документ в статусе POSTED");
        return;
    }

    if (!cancelDocumentSql(docId)) {
        return;
    }

    QMessageBox::information(this, "Успех", "ТТН отменена (storno движения добавлены)");
    refreshModel();
    updateButtonsByStatus();
}

void TTNWidget::onDeleteClicked()
{
    const int docId = selectedDocId();
    if (docId <= 0) {
        QMessageBox::warning(this, "Удаление", "Выберите ТТН");
        return;
    }

    const QString status = selectedDocStatus();

    QString text;
    if (status == "DRAFT") {
        text = "Удалить черновик ТТН? Действие необратимо.";
    } else if (status == "POSTED") {
        text = "Удалить проведённую ТТН нельзя без корректировки склада.\n"
               "Будет выполнена ОТМЕНА (storno движений) и статус станет CANCELLED.\n\nПродолжить?";
    } else if (status == "CANCELLED") {
        text = "Скрыть отменённую ТТН из списка? (soft-delete, склад не трогаем)";
    } else {
        QMessageBox::warning(this, "Удаление", "Неизвестный статус документа");
        return;
    }

    if (QMessageBox::question(this, "Удаление", text,
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    if (!deleteTtnSql(docId)) {
        return;
    }

    QMessageBox::information(this, "Удаление", "Готово");
    refreshModel();
    updateButtonsByStatus();
}

void TTNWidget::onRefreshClicked()
{
    refreshModel();
    updateButtonsByStatus();
}

// ---------------------------
// SQL helpers
// ---------------------------

double TTNWidget::stockBalanceForProduct(int productId)
{
    QSqlDatabase db = DbManager::instance().database();
    QSqlQuery q(db);

    // Остаток рассчитывается по сумме всех движений, включая storno (cancelled_flag = 1).
    // Это нужно для корректного "отката" (storno) при отмене проведённого документа.
    q.prepare("SELECT COALESCE(SUM(qty_delta_kg), 0) "
              "FROM inventory_movements WHERE product_id = :pid");
    q.bindValue(":pid", productId);

    if (!q.exec() || !q.next()) {
        return 0.0;
    }

    return q.value(0).toDouble();
}

bool TTNWidget::documentIsTransferDraftOrPosted(int documentId, QString* outStatus)
{
    QSqlDatabase db = DbManager::instance().database();
    QSqlQuery q(db);

    q.prepare("SELECT status FROM documents WHERE id = :id AND doc_type = 'transfer'");
    q.bindValue(":id", documentId);

    if (!q.exec()) return false;
    if (!q.next()) return false;

    const QString status = q.value(0).toString();
    if (outStatus) *outStatus = status;
    return (status == "DRAFT" || status == "POSTED" || status == "CANCELLED");
}

bool TTNWidget::postDocumentSql(int documentId)
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return false;
    }

    QString status;
    if (!documentIsTransferDraftOrPosted(documentId, &status)) {
        QMessageBox::warning(this, "Ошибка", "Документ не найден или не transfer");
        return false;
    }

    if (status != "DRAFT") {
        QMessageBox::warning(this, "Ошибка", "Документ не в статусе DRAFT");
        return false;
    }

    QString docDate;
    {
        QSqlQuery q(db);
        q.prepare("SELECT date FROM documents WHERE id = :id");
        q.bindValue(":id", documentId);
        if (!q.exec() || !q.next()) {
            QMessageBox::critical(this, "Ошибка БД", q.lastError().text());
            return false;
        }
        docDate = q.value(0).toString();
        if (docDate.isEmpty()) docDate = QDate::currentDate().toString("yyyy-MM-dd");
    }

    struct Line { int productId; double qty; };
    QList<Line> lines;

    {
        QSqlQuery q(db);
        q.prepare("SELECT product_id, qty_kg FROM document_lines WHERE document_id = :id");
        q.bindValue(":id", documentId);
        if (!q.exec()) {
            QMessageBox::critical(this, "Ошибка БД", q.lastError().text());
            return false;
        }
        while (q.next()) {
            lines.push_back(Line{ q.value(0).toInt(), q.value(1).toDouble() });
        }
    }

    if (lines.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "В документе нет строк товаров");
        return false;
    }

    for (const auto& l : lines) {
        const double bal = stockBalanceForProduct(l.productId);
        if (bal + 1e-9 < l.qty) {
            QMessageBox::warning(this, "Недостаточно товара",
                                 QString("Недостаточно остатка по товару ID=%1.\nОстаток: %2, требуется: %3")
                                     .arg(l.productId)
                                     .arg(QString::number(bal, 'f', 3))
                                     .arg(QString::number(l.qty, 'f', 3)));
            return false;
        }
    }

    if (!db.transaction()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось начать транзакцию");
        return false;
    }

    {
        QSqlQuery ins(db);
        ins.prepare("INSERT INTO inventory_movements (document_id, product_id, qty_delta_kg, movement_date, cancelled_flag) "
                    "VALUES (:doc, :pid, :qty, :dt, 0)");

        for (const auto& l : lines) {
            ins.bindValue(":doc", documentId);
            ins.bindValue(":pid", l.productId);
            ins.bindValue(":qty", -l.qty);
            ins.bindValue(":dt", docDate);

            if (!ins.exec()) {
                db.rollback();
                QMessageBox::critical(this, "Ошибка БД", ins.lastError().text());
                return false;
            }
        }
    }

    {
        QSqlQuery upd(db);
        upd.prepare("UPDATE documents SET status='POSTED', updated_at=datetime('now') WHERE id=:id");
        upd.bindValue(":id", documentId);

        if (!upd.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", upd.lastError().text());
            return false;
        }
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::warning(this, "Ошибка", "Не удалось зафиксировать транзакцию");
        return false;
    }

    return true;
}

bool TTNWidget::cancelDocumentSql(int documentId)
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return false;
    }

    QString status;
    if (!documentIsTransferDraftOrPosted(documentId, &status)) {
        QMessageBox::warning(this, "Ошибка", "Документ не найден или не transfer");
        return false;
    }

    if (status != "POSTED") {
        QMessageBox::warning(this, "Ошибка", "Отменить можно только POSTED");
        return false;
    }

    if (!db.transaction()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось начать транзакцию");
        return false;
    }

    // Упрощенная и корректная отмена для остатков:
    // 1) помечаем движения этого документа как cancelled_flag = 1
    // 2) НЕ вставляем storno-движения
    // Баланс в UI считается только по cancelled_flag = 0,
    // поэтому эффект документа полностью исчезает из остатков,
    // и товар «возвращается».
    {
        QSqlQuery updMov(db);
        updMov.prepare(
            "UPDATE inventory_movements "
            "SET cancelled_flag = 1 "
            "WHERE document_id = :id AND cancelled_flag = 0"
        );
        updMov.bindValue(":id", documentId);

        if (!updMov.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", updMov.lastError().text());
            return false;
        }

        if (updMov.numRowsAffected() <= 0) {
            db.rollback();
            QMessageBox::warning(this, "Ошибка", "Нет движений для отмены (или уже отменено)");
            return false;
        }
    }

    {
        QSqlQuery upd(db);
        upd.prepare("UPDATE documents SET status='CANCELLED', updated_at=datetime('now') WHERE id=:id");
        upd.bindValue(":id", documentId);

        if (!upd.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", upd.lastError().text());
            return false;
        }
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::warning(this, "Ошибка", "Не удалось зафиксировать транзакцию");
        return false;
    }

    return true;
}

bool TTNWidget::deleteTtnSql(int documentId)
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return false;
    }

    QString status;
    if (!documentIsTransferDraftOrPosted(documentId, &status)) {
        QMessageBox::warning(this, "Ошибка", "Документ не найден или не transfer");
        return false;
    }

    // POSTED: “удаление” = корректная отмена (storno)
    if (status == "POSTED") {
        return cancelDocumentSql(documentId);
    }

    // CANCELLED: soft-delete (скрыть)
    if (status == "CANCELLED") {
        QSqlQuery q(db);
        // В таблице documents действует UNIQUE(number, doc_type). Даже при soft-delete номер остаётся занятым.
        // Поэтому при скрытии добавляем суффикс к номеру, чтобы освободить исходное значение.
        q.prepare(
            "UPDATE documents "
            "SET is_deleted = 1, "
            "    number = CASE WHEN instr(number, '[del ') = 0 THEN number || ' [del ' || id || ']' ELSE number END, "
            "    updated_at = datetime('now') "
            "WHERE id = :id AND doc_type = 'transfer'"
        );
        q.bindValue(":id", documentId);

        if (!q.exec()) {
            QMessageBox::critical(this, "Ошибка БД", q.lastError().text());
            return false;
        }
        if (q.numRowsAffected() == 0) {
            QMessageBox::warning(this, "Удаление", "Документ не найден");
            return false;
        }
        return true;
    }

    // DRAFT: физическое удаление (движений быть не должно)
    if (status != "DRAFT") {
        QMessageBox::warning(this, "Удаление", "Нельзя удалить документ в этом статусе");
        return false;
    }

    // Проверим, что движений нет
    {
        QSqlQuery q(db);
        q.prepare("SELECT COUNT(1) FROM inventory_movements WHERE document_id = :id");
        q.bindValue(":id", documentId);
        if (!q.exec() || !q.next()) {
            QMessageBox::critical(this, "Ошибка БД", q.lastError().text());
            return false;
        }
        if (q.value(0).toInt() > 0) {
            QMessageBox::warning(this, "Удаление",
                                 "Нельзя удалить DRAFT: у документа уже есть движения склада.\n"
                                 "Используйте отмену.");
            return false;
        }
    }

    if (!db.transaction()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось начать транзакцию");
        return false;
    }

    {
        QSqlQuery q(db);
        q.prepare("DELETE FROM document_lines WHERE document_id = :id");
        q.bindValue(":id", documentId);
        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", q.lastError().text());
            return false;
        }
    }

    {
        QSqlQuery q(db);
        q.prepare("DELETE FROM documents WHERE id = :id AND doc_type = 'transfer'");
        q.bindValue(":id", documentId);
        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", q.lastError().text());
            return false;
        }
        if (q.numRowsAffected() == 0) {
            db.rollback();
            QMessageBox::warning(this, "Удаление", "Документ не найден");
            return false;
        }
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::warning(this, "Ошибка", "Не удалось зафиксировать транзакцию");
        return false;
    }

    return true;
}
