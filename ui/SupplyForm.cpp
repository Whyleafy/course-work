#include "SupplyForm.h"

#include "DbManager.h"
#include "DocumentService.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QDoubleSpinBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

static QString safeText(const QString& s) { return s.trimmed(); }

SupplyForm::SupplyForm(DocumentService* docService, QWidget* parent)
    : QDialog(parent)
    , m_docService(docService)
{
    setupUi();
    reloadProducts();
    reloadCounterparties();
    loadData(0);
}

void SupplyForm::setupUi()
{
    setWindowTitle("Поставка");
    setModal(true);
    resize(900, 650);

    auto* mainLayout = new QVBoxLayout(this);

    auto* headerGroup = new QGroupBox("Шапка документа", this);
    auto* headerForm = new QFormLayout(headerGroup);

    m_numberEdit = new QLineEdit(this);
    m_numberEdit->setPlaceholderText("Например: П-000001");
    headerForm->addRow("Номер:", m_numberEdit);

    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDate(QDate::currentDate());
    headerForm->addRow("Дата:", m_dateEdit);

    m_senderCombo = new QComboBox(this);
    headerForm->addRow("Поставщик:", m_senderCombo);

    mainLayout->addWidget(headerGroup);

    auto* linesGroup = new QGroupBox("Товары", this);
    auto* linesLayout = new QVBoxLayout(linesGroup);

    m_linesTable = new QTableWidget(this);
    m_linesTable->setColumnCount(5);
    m_linesTable->setHorizontalHeaderLabels(QStringList()
        << "Товар" << "Кол-во (кг)" << "Цена" << "Сумма" << "Ед.");
    m_linesTable->horizontalHeader()->setStretchLastSection(true);
    m_linesTable->verticalHeader()->setVisible(false);
    m_linesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_linesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_linesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_linesTable->setAlternatingRowColors(true);

    m_linesTable->setColumnWidth(0, 360);
    m_linesTable->setColumnWidth(1, 140);
    m_linesTable->setColumnWidth(2, 120);
    m_linesTable->setColumnWidth(3, 140);
    m_linesTable->setColumnWidth(4, 60);

    linesLayout->addWidget(m_linesTable);

    auto* linesBtnRow = new QHBoxLayout();
    m_addLineBtn = new QPushButton("Добавить строку", this);
    m_removeLineBtn = new QPushButton("Удалить строку", this);
    linesBtnRow->addWidget(m_addLineBtn);
    linesBtnRow->addWidget(m_removeLineBtn);
    linesBtnRow->addStretch();
    linesLayout->addLayout(linesBtnRow);

    mainLayout->addWidget(linesGroup);

    auto* footerRow = new QHBoxLayout();
    footerRow->addStretch();
    footerRow->addWidget(new QLabel("Итого:", this));
    m_totalLabel = new QLabel("0.00", this);
    m_totalLabel->setMinimumWidth(140);
    m_totalLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    footerRow->addWidget(m_totalLabel);
    mainLayout->addLayout(footerRow);

    auto* buttonsRow = new QHBoxLayout();
    buttonsRow->addStretch();

    m_saveBtn = new QPushButton("Сохранить", this);
    m_postBtn = new QPushButton("Провести", this);
    m_cancelBtn = new QPushButton("Отменить (storno)", this);
    m_closeBtn = new QPushButton("Закрыть", this);

    buttonsRow->addWidget(m_saveBtn);
    buttonsRow->addWidget(m_postBtn);
    buttonsRow->addWidget(m_cancelBtn);
    buttonsRow->addWidget(m_closeBtn);

    mainLayout->addLayout(buttonsRow);

    connect(m_addLineBtn, &QPushButton::clicked, this, &SupplyForm::onAddLineClicked);
    connect(m_removeLineBtn, &QPushButton::clicked, this, &SupplyForm::onRemoveLineClicked);

    connect(m_saveBtn, &QPushButton::clicked, this, &SupplyForm::onSaveClicked);
    connect(m_postBtn, &QPushButton::clicked, this, &SupplyForm::onPostClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &SupplyForm::onCancelDocumentClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SupplyForm::reloadProducts()
{
    m_products.clear();

    QSqlDatabase db = DbManager::instance().database();
    QSqlQuery q(db);
    if (!q.exec("SELECT id, name, price, unit FROM products WHERE is_active = 1 ORDER BY sort, name")) {
        QMessageBox::warning(this, "Ошибка БД", "Не удалось загрузить товары:\n" + q.lastError().text());
        return;
    }

    while (q.next()) {
        ProductItem p;
        p.id = q.value("id").toInt();
        p.name = q.value("name").toString();
        p.price = q.value("price").toDouble();
        p.unit = q.value("unit").toString();
        m_products.append(p);
    }
}

void SupplyForm::reloadCounterparties()
{
    m_senderCombo->clear();

    QSqlDatabase db = DbManager::instance().database();
    QSqlQuery q(db);
    // для поставки логично показывать supplier/both
    if (!q.exec("SELECT id, name, type FROM counterparties WHERE is_active = 1 AND type IN ('supplier','both') ORDER BY name")) {
        QMessageBox::warning(this, "Ошибка БД", "Не удалось загрузить контрагентов:\n" + q.lastError().text());
        return;
    }

    while (q.next()) {
        const int id = q.value("id").toInt();
        const QString name = q.value("name").toString();
        m_senderCombo->addItem(name, id);
    }
}

void SupplyForm::setUiReadOnly(bool ro)
{
    m_numberEdit->setReadOnly(ro);
    m_dateEdit->setEnabled(!ro);
    m_senderCombo->setEnabled(!ro);

    m_addLineBtn->setEnabled(!ro);
    m_removeLineBtn->setEnabled(!ro);

    // таблица: комбо/спинбоксы
    for (int r = 0; r < m_linesTable->rowCount(); ++r) {
        if (auto* cb = qobject_cast<QComboBox*>(m_linesTable->cellWidget(r, 0)))
            cb->setEnabled(!ro);
        if (auto* sp = qobject_cast<QDoubleSpinBox*>(m_linesTable->cellWidget(r, 1)))
            sp->setEnabled(!ro);
    }
}

void SupplyForm::loadData(int documentId)
{
    m_documentId = documentId;

    // режим "Добавить"
    if (m_documentId <= 0) {
        m_status = "DRAFT";
        setWindowTitle("Добавить поставку");

        m_numberEdit->clear();
        m_dateEdit->setDate(QDate::currentDate());
        if (m_senderCombo->count() > 0) m_senderCombo->setCurrentIndex(0);

        m_linesTable->setRowCount(0);
        onAddLineClicked(); // хотя бы 1 строка
        recalcTotals();

        m_postBtn->setEnabled(false);   // нельзя провести пока нет сохраненного id
        m_cancelBtn->setEnabled(false); // нечего отменять
        setUiReadOnly(false);
        return;
    }

    // режим "Редактировать"
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return;
    }

    // documents
    {
        QSqlQuery q(db);
        q.prepare("SELECT number, date, status, sender_id, total_amount FROM documents WHERE id = :id AND doc_type='supply'");
        q.bindValue(":id", m_documentId);

        if (!q.exec()) {
            QMessageBox::critical(this, "Ошибка БД", "Не удалось загрузить поставку:\n" + q.lastError().text());
            return;
        }
        if (!q.next()) {
            QMessageBox::warning(this, "Не найдено", "Документ поставки не найден");
            return;
        }

        m_numberEdit->setText(q.value("number").toString());
        m_dateEdit->setDate(QDate::fromString(q.value("date").toString(), Qt::ISODate));
        m_status = q.value("status").toString();

        const int senderId = q.value("sender_id").toInt();
        int idx = m_senderCombo->findData(senderId);
        if (idx >= 0) m_senderCombo->setCurrentIndex(idx);

        m_totalLabel->setText(QString::number(q.value("total_amount").toDouble(), 'f', 2));
    }

    // lines
    m_linesTable->setRowCount(0);
    {
        QSqlQuery q(db);
        q.prepare(R"(
            SELECT dl.product_id, dl.qty_kg, dl.price, dl.line_sum, p.unit
            FROM document_lines dl
            LEFT JOIN products p ON p.id = dl.product_id
            WHERE dl.document_id = :doc
            ORDER BY dl.id
        )");
        q.bindValue(":doc", m_documentId);

        if (!q.exec()) {
            QMessageBox::warning(this, "Ошибка БД", "Не удалось загрузить строки:\n" + q.lastError().text());
        } else {
            while (q.next()) {
                const int productId = q.value(0).toInt();
                const double qty = q.value(1).toDouble();
                const double price = q.value(2).toDouble();
                const double sum = q.value(3).toDouble();
                const QString unit = q.value(4).toString();

                const int r = m_linesTable->rowCount();
                m_linesTable->insertRow(r);

                // product combo
                auto* cb = new QComboBox(this);
                for (const auto& p : m_products) cb->addItem(p.name, p.id);
                int pidx = cb->findData(productId);
                if (pidx >= 0) cb->setCurrentIndex(pidx);
                m_linesTable->setCellWidget(r, 0, cb);

                // qty
                auto* sp = new QDoubleSpinBox(this);
                sp->setDecimals(3);
                sp->setMinimum(0.0);
                sp->setMaximum(1e9);
                sp->setValue(qty);
                m_linesTable->setCellWidget(r, 1, sp);

                // price, sum, unit (readonly items)
                m_linesTable->setItem(r, 2, new QTableWidgetItem(QString::number(price, 'f', 2)));
                m_linesTable->setItem(r, 3, new QTableWidgetItem(QString::number(sum, 'f', 2)));
                m_linesTable->setItem(r, 4, new QTableWidgetItem(unit));

                for (int c : {2,3,4}) {
                    m_linesTable->item(r, c)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                    m_linesTable->item(r, c)->setFlags(m_linesTable->item(r, c)->flags() & ~Qt::ItemIsEditable);
                }

                connect(cb, &QComboBox::currentIndexChanged, this, &SupplyForm::onLineChanged);
                connect(sp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SupplyForm::onLineChanged);
            }
        }
    }

    if (m_linesTable->rowCount() == 0)
        onAddLineClicked();

    recalcTotals();

    const bool isDraft = (m_status == "DRAFT");
    m_postBtn->setEnabled(isDraft);
    m_cancelBtn->setEnabled(m_status == "POSTED");
    setUiReadOnly(!isDraft);

    setWindowTitle(QString("Поставка %1 (%2)").arg(m_numberEdit->text(), m_status));
}

void SupplyForm::onAddLineClicked()
{
    const int r = m_linesTable->rowCount();
    m_linesTable->insertRow(r);

    auto* cb = new QComboBox(this);
    for (const auto& p : m_products) cb->addItem(p.name, p.id);
    m_linesTable->setCellWidget(r, 0, cb);

    auto* sp = new QDoubleSpinBox(this);
    sp->setDecimals(3);
    sp->setMinimum(0.0);
    sp->setMaximum(1e9);
    sp->setValue(0.0);
    m_linesTable->setCellWidget(r, 1, sp);

    m_linesTable->setItem(r, 2, new QTableWidgetItem("0.00"));
    m_linesTable->setItem(r, 3, new QTableWidgetItem("0.00"));
    m_linesTable->setItem(r, 4, new QTableWidgetItem("кг"));

    for (int c : {2,3,4}) {
        m_linesTable->item(r, c)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_linesTable->item(r, c)->setFlags(m_linesTable->item(r, c)->flags() & ~Qt::ItemIsEditable);
    }

    connect(cb, &QComboBox::currentIndexChanged, this, &SupplyForm::onLineChanged);
    connect(sp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SupplyForm::onLineChanged);

    recalcTotals();
}

void SupplyForm::onRemoveLineClicked()
{
    const int r = m_linesTable->currentRow();
    if (r < 0) return;

    m_linesTable->removeRow(r);

    if (m_linesTable->rowCount() == 0)
        onAddLineClicked();

    recalcTotals();
}

void SupplyForm::onLineChanged()
{
    recalcTotals();
}

int SupplyForm::currentSelectedProductId(int row) const
{
    if (auto* cb = qobject_cast<QComboBox*>(m_linesTable->cellWidget(row, 0)))
        return cb->currentData().toInt();
    return 0;
}

double SupplyForm::currentQty(int row) const
{
    if (auto* sp = qobject_cast<QDoubleSpinBox*>(m_linesTable->cellWidget(row, 1)))
        return sp->value();
    return 0.0;
}

double SupplyForm::productPriceById(int productId) const
{
    for (const auto& p : m_products)
        if (p.id == productId) return p.price;
    return 0.0;
}

QString SupplyForm::productNameById(int productId) const
{
    for (const auto& p : m_products)
        if (p.id == productId) return p.name;
    return {};
}

void SupplyForm::recalcTotals()
{
    double total = 0.0;

    for (int r = 0; r < m_linesTable->rowCount(); ++r) {
        const int productId = currentSelectedProductId(r);
        const double qty = currentQty(r);

        const double price = productPriceById(productId);
        const double sum = price * qty;

        // unit
        QString unit = "кг";
        for (const auto& p : m_products) {
            if (p.id == productId) { unit = p.unit; break; }
        }

        if (!m_linesTable->item(r, 2)) m_linesTable->setItem(r, 2, new QTableWidgetItem());
        if (!m_linesTable->item(r, 3)) m_linesTable->setItem(r, 3, new QTableWidgetItem());
        if (!m_linesTable->item(r, 4)) m_linesTable->setItem(r, 4, new QTableWidgetItem());

        m_linesTable->item(r, 2)->setText(QString::number(price, 'f', 2));
        m_linesTable->item(r, 3)->setText(QString::number(sum, 'f', 2));
        m_linesTable->item(r, 4)->setText(unit);

        for (int c : {2,3,4}) {
            m_linesTable->item(r, c)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_linesTable->item(r, c)->setFlags(m_linesTable->item(r, c)->flags() & ~Qt::ItemIsEditable);
        }

        total += sum;
    }

    m_totalLabel->setText(QString::number(total, 'f', 2));
}

bool SupplyForm::validateForm()
{
    const QString number = safeText(m_numberEdit->text());
    if (number.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите номер документа");
        return false;
    }

    if (m_senderCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите поставщика");
        return false;
    }

    bool hasAnyQty = false;
    for (int r = 0; r < m_linesTable->rowCount(); ++r) {
        const int pid = currentSelectedProductId(r);
        const double qty = currentQty(r);
        if (pid <= 0) {
            QMessageBox::warning(this, "Ошибка", QString("В строке %1 не выбран товар").arg(r + 1));
            return false;
        }
        if (qty < 0.000001) continue;
        hasAnyQty = true;
    }

    if (!hasAnyQty) {
        QMessageBox::warning(this, "Ошибка", "Добавьте хотя бы одну строку с количеством > 0");
        return false;
    }

    return true;
}

bool SupplyForm::deleteLines(int documentId)
{
    QSqlDatabase db = DbManager::instance().database();
    QSqlQuery q(db);
    q.prepare("DELETE FROM document_lines WHERE document_id = :doc");
    q.bindValue(":doc", documentId);
    if (!q.exec()) {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось удалить строки:\n" + q.lastError().text());
        return false;
    }
    return true;
}

bool SupplyForm::insertLines(int documentId)
{
    QSqlDatabase db = DbManager::instance().database();

    for (int r = 0; r < m_linesTable->rowCount(); ++r) {
        const int productId = currentSelectedProductId(r);
        const double qty = currentQty(r);
        if (qty < 0.000001) continue;

        const double price = productPriceById(productId);
        const double sum = price * qty;

        QSqlQuery q(db);
        q.prepare(R"(
            INSERT INTO document_lines (document_id, product_id, qty_kg, price, line_sum)
            VALUES (:doc, :pid, :qty, :price, :sum)
        )");
        q.bindValue(":doc", documentId);
        q.bindValue(":pid", productId);
        q.bindValue(":qty", qty);
        q.bindValue(":price", price);
        q.bindValue(":sum", sum);

        if (!q.exec()) {
            QMessageBox::critical(this, "Ошибка БД", "Не удалось вставить строку:\n" + q.lastError().text());
            return false;
        }
    }

    return true;
}

bool SupplyForm::createDocument()
{
    QSqlDatabase db = DbManager::instance().database();

    const QString number = safeText(m_numberEdit->text());
    const QString dateIso = m_dateEdit->date().toString(Qt::ISODate);
    const int senderId = m_senderCombo->currentData().toInt();
    const double total = m_totalLabel->text().toDouble();

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO documents (doc_type, number, date, status, sender_id, receiver_id, total_amount, notes)
        VALUES ('supply', :number, :date, 'DRAFT', :sender, NULL, :total, NULL)
    )");
    q.bindValue(":number", number);
    q.bindValue(":date", dateIso);
    q.bindValue(":sender", senderId);
    q.bindValue(":total", total);

    if (!q.exec()) {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось создать документ:\n" + q.lastError().text());
        return false;
    }

    m_documentId = q.lastInsertId().toInt();
    m_status = "DRAFT";
    return true;
}

bool SupplyForm::updateDocument()
{
    QSqlDatabase db = DbManager::instance().database();

    const QString number = safeText(m_numberEdit->text());
    const QString dateIso = m_dateEdit->date().toString(Qt::ISODate);
    const int senderId = m_senderCombo->currentData().toInt();
    const double total = m_totalLabel->text().toDouble();

    QSqlQuery q(db);
    q.prepare(R"(
        UPDATE documents
        SET number = :number,
            date = :date,
            sender_id = :sender,
            total_amount = :total,
            updated_at = datetime('now')
        WHERE id = :id AND doc_type='supply'
    )");
    q.bindValue(":number", number);
    q.bindValue(":date", dateIso);
    q.bindValue(":sender", senderId);
    q.bindValue(":total", total);
    q.bindValue(":id", m_documentId);

    if (!q.exec()) {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось обновить документ:\n" + q.lastError().text());
        return false;
    }

    if (q.numRowsAffected() == 0) {
        QMessageBox::warning(this, "Ошибка", "Документ не обновлен (возможно не найден)");
        return false;
    }

    return true;
}

bool SupplyForm::saveToDb()
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return false;
    }

    if (!db.transaction()) {
        QMessageBox::warning(this, "Предупреждение", "Не удалось начать транзакцию");
    }

    bool ok = true;

    if (m_documentId <= 0) {
        ok = createDocument();
    } else {
        ok = updateDocument();
    }

    if (ok) ok = deleteLines(m_documentId);
    if (ok) ok = insertLines(m_documentId);

    if (!ok) {
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        QMessageBox::critical(this, "Ошибка БД", "Не удалось зафиксировать транзакцию:\n" + db.lastError().text());
        db.rollback();
        return false;
    }

    return true;
}

void SupplyForm::onSaveClicked()
{
    if (!validateForm()) return;

    recalcTotals();

    if (!saveToDb()) return;

    QMessageBox::information(this, "Успех", "Поставка сохранена");
    emit saved();

    // теперь можно проводить
    m_postBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);

    setWindowTitle(QString("Поставка %1 (DRAFT)").arg(m_numberEdit->text()));
}

void SupplyForm::onPostClicked()
{
    if (!m_docService) {
        QMessageBox::critical(this, "Ошибка", "DocumentService не передан");
        return;
    }

    if (m_documentId <= 0) {
        QMessageBox::warning(this, "Ошибка", "Сначала сохраните документ");
        return;
    }

    // перед проведением убедимся что сохранено
    if (!validateForm()) return;
    recalcTotals();
    if (!saveToDb()) return;

    if (!m_docService->postDocument(m_documentId)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось провести документ (проверьте логи и остатки/товары)");
        return;
    }

    m_status = "POSTED";
    QMessageBox::information(this, "Успех", "Документ проведен. Остатки увеличены.");

    m_postBtn->setEnabled(false);
    m_cancelBtn->setEnabled(true);
    setUiReadOnly(true);

    loadData(m_documentId); // подтянет статус/сумму
}

void SupplyForm::onCancelDocumentClicked()
{
    if (!m_docService) {
        QMessageBox::critical(this, "Ошибка", "DocumentService не передан");
        return;
    }

    if (m_documentId <= 0) return;

    if (QMessageBox::question(this, "Подтвердите", "Отменить документ (storno)?") != QMessageBox::Yes)
        return;

    if (!m_docService->cancelDocument(m_documentId)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось отменить документ");
        return;
    }

    QMessageBox::information(this, "Успех", "Документ отменен (storno сделано).");
    loadData(m_documentId);
}
