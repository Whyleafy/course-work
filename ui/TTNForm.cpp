#include "TTNForm.h"
#include "DbManager.h"
#include "DecimalUtils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDate>


static double toDoubleSafe(const QString& s)
{
    bool ok = false;
    double v = s.trimmed().replace(',', '.').toDouble(&ok);
    return ok ? v : 0.0;
}

static Decimal toDecimalSafe(const QString& s)
{
    return decimalFromString(s);
}

static QString money(const Decimal& v)
{
    return decimalToString(v);
}

TtnForm::TtnForm(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    loadCounterparties();
    loadProducts();
}

void TtnForm::setupUi()
{
    setWindowTitle("ТТН");
    setModal(true);
    resize(820, 640);

    auto* mainLayout = new QVBoxLayout(this);

    auto* headerGroup = new QGroupBox("Шапка документа", this);
    auto* headerForm = new QFormLayout(headerGroup);

    m_numberEdit = new QLineEdit(this);
    headerForm->addRow("Номер:", m_numberEdit);

    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    headerForm->addRow("Дата:", m_dateEdit);

    m_senderCombo = new QComboBox(this);
    headerForm->addRow("Отправитель:", m_senderCombo);

    m_receiverCombo = new QComboBox(this);
    headerForm->addRow("Получатель:", m_receiverCombo);

    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(80);
    headerForm->addRow("Примечание:", m_notesEdit);

    mainLayout->addWidget(headerGroup);

    auto* linesGroup = new QGroupBox("Товары", this);
    auto* linesLayout = new QVBoxLayout(linesGroup);

    m_linesTable = new QTableWidget(this);
    m_linesTable->setColumnCount(4);
    m_linesTable->setHorizontalHeaderLabels(QStringList()
                                            << "Товар"
                                            << "Кол-во (кг)"
                                            << "Цена"
                                            << "Сумма");
    m_linesTable->horizontalHeader()->setStretchLastSection(true);
    m_linesTable->verticalHeader()->setVisible(false);
    m_linesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_linesTable->setSelectionMode(QAbstractItemView::SingleSelection);

    linesLayout->addWidget(m_linesTable);

    auto* linesButtons = new QHBoxLayout();
    m_addLineButton = new QPushButton("Добавить строку", this);
    m_removeLineButton = new QPushButton("Удалить строку", this);
    linesButtons->addWidget(m_addLineButton);
    linesButtons->addWidget(m_removeLineButton);
    linesButtons->addStretch();

    m_totalLabel = new QLabel("Итого: 0.00", this);
    linesButtons->addWidget(m_totalLabel);

    linesLayout->addLayout(linesButtons);
    mainLayout->addWidget(linesGroup);

    auto* footer = new QHBoxLayout();
    footer->addStretch();

    m_saveButton = new QPushButton("Сохранить", this);
    m_cancelButton = new QPushButton("Отмена", this);
    footer->addWidget(m_saveButton);
    footer->addWidget(m_cancelButton);

    mainLayout->addLayout(footer);

    connect(m_addLineButton, &QPushButton::clicked, this, &TtnForm::onAddLine);
    connect(m_removeLineButton, &QPushButton::clicked, this, &TtnForm::onRemoveLine);
    connect(m_saveButton, &QPushButton::clicked, this, &TtnForm::onSaveClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &TtnForm::onCancelClicked);

    connect(m_linesTable, &QTableWidget::cellChanged, this, &TtnForm::onRecalcTotals);
}

bool TtnForm::loadCounterparties()
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) return false;

    m_senderCombo->clear();
    m_receiverCombo->clear();

    QSqlQuery q(db);
    if (!q.exec("SELECT id, name, type FROM counterparties WHERE is_active = 1 ORDER BY name")) {
        QMessageBox::warning(this, "Ошибка БД", q.lastError().text());
        return false;
    }

    while (q.next()) {
        const int id = q.value(0).toInt();
        const QString name = q.value(1).toString();
        const QString type = q.value(2).toString();

        if (type == "supplier" || type == "both") {
            m_senderCombo->addItem(name, id);
        }
        if (type == "customer" || type == "both") {
            m_receiverCombo->addItem(name, id);
        }
    }

    return true;
}

bool TtnForm::loadProducts()
{
    return true;
}

void TtnForm::onAddLine()
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return;
    }

    QList<QPair<int, QString>> products;
    {
        QSqlQuery q(db);
        if (!q.exec("SELECT id, name FROM products WHERE is_active = 1 ORDER BY name")) {
            QMessageBox::warning(this, "Ошибка БД", q.lastError().text());
            return;
        }
        while (q.next()) products.append({q.value(0).toInt(), q.value(1).toString()});
    }

    if (products.isEmpty()) {
        QMessageBox::warning(this, "Нет данных", "Нет активных товаров");
        return;
    }

    const int row = m_linesTable->rowCount();
    m_linesTable->blockSignals(true);
    m_linesTable->insertRow(row);

    auto* combo = new QComboBox(this);
    for (auto& p : products) combo->addItem(p.second, p.first);
    m_linesTable->setCellWidget(row, 0, combo);

    auto* qtyItem = new QTableWidgetItem("0");
    qtyItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_linesTable->setItem(row, 1, qtyItem);

    Decimal price = 0;
    {
        QSqlQuery qp(db);
        qp.prepare("SELECT price FROM products WHERE id = :id");
        qp.bindValue(":id", combo->currentData());
        if (qp.exec() && qp.next()) price = decimalFromVariant(qp.value(0));
    }
    auto* priceItem = new QTableWidgetItem(money(price));
    priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_linesTable->setItem(row, 2, priceItem);

    auto* sumItem = new QTableWidgetItem("0.00");
    sumItem->setFlags(sumItem->flags() & ~Qt::ItemIsEditable);
    sumItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_linesTable->setItem(row, 3, sumItem);

    connect(combo, &QComboBox::currentIndexChanged, this, [this, row]() {
        QSqlDatabase db = DbManager::instance().database();
        if (!db.isOpen()) return;
        auto* combo = qobject_cast<QComboBox*>(m_linesTable->cellWidget(row, 0));
        if (!combo) return;

        QSqlQuery qp(db);
        qp.prepare("SELECT price FROM products WHERE id = :id");
        qp.bindValue(":id", combo->currentData());
        if (qp.exec() && qp.next()) {
            const Decimal price = decimalFromVariant(qp.value(0));
            m_linesTable->blockSignals(true);
            m_linesTable->item(row, 2)->setText(money(price));
            m_linesTable->blockSignals(false);
            onRecalcTotals();
        }
    });

    m_linesTable->blockSignals(false);
    onRecalcTotals();
}

void TtnForm::onRemoveLine()
{
    const int row = m_linesTable->currentRow();
    if (row < 0) return;
    m_linesTable->removeRow(row);
    onRecalcTotals();
}

void TtnForm::onRecalcTotals()
{
    Decimal total = 0;

    m_linesTable->blockSignals(true);
    for (int r = 0; r < m_linesTable->rowCount(); ++r) {
        const double qty = toDoubleSafe(m_linesTable->item(r, 1)->text());
        const Decimal price = toDecimalSafe(m_linesTable->item(r, 2)->text());
        const Decimal sum = price * Decimal(qty);
        m_linesTable->item(r, 3)->setText(money(sum));
        total += sum;
    }
    m_linesTable->blockSignals(false);

    m_totalLabel->setText("Итого: " + money(total));
}

void TtnForm::loadData(int docId)
{
    m_docId = docId;

    m_numberEdit->clear();
    m_dateEdit->setDate(QDate::currentDate());
    m_notesEdit->clear();
    m_linesTable->setRowCount(0);
    m_totalLabel->setText("Итого: 0.00");

    if (m_docId <= 0) {
        setWindowTitle("Добавить ТТН");
        return;
    }

    setWindowTitle(QString("Редактировать ТТН (ID %1)").arg(m_docId));

    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return;
    }

    {
        QSqlQuery q(db);
        q.prepare("SELECT number, date, status, sender_id, receiver_id, notes "
                  "FROM documents WHERE id = :id AND doc_type='transfer'");
        q.bindValue(":id", m_docId);

        if (!q.exec()) {
            QMessageBox::critical(this, "Ошибка БД", q.lastError().text());
            return;
        }
        if (!q.next()) {
            QMessageBox::warning(this, "Не найдено", "ТТН не найдена");
            return;
        }

        m_numberEdit->setText(q.value(0).toString());
        m_dateEdit->setDate(QDate::fromString(q.value(1).toString(), "yyyy-MM-dd"));

        const int senderId = q.value(3).toInt();
        const int receiverId = q.value(4).toInt();

        int idxS = m_senderCombo->findData(senderId);
        if (idxS >= 0) m_senderCombo->setCurrentIndex(idxS);

        int idxR = m_receiverCombo->findData(receiverId);
        if (idxR >= 0) m_receiverCombo->setCurrentIndex(idxR);

        m_notesEdit->setPlainText(q.value(5).toString());
    }

    QSet<int> docProductIds;
    {
        QSqlQuery q(db);
        q.prepare("SELECT DISTINCT product_id FROM document_lines WHERE document_id = :id");
        q.bindValue(":id", m_docId);
        if (q.exec()) {
            while (q.next()) docProductIds.insert(q.value(0).toInt());
        }
    }

    QList<QPair<int, QString>> products;
    {
        QSqlQuery q(db);
        q.prepare(R"(
            SELECT id, name
            FROM products
            WHERE is_active = 1 OR id IN (SELECT DISTINCT product_id FROM document_lines WHERE document_id = :doc)
            ORDER BY name
        )");
        q.bindValue(":doc", m_docId);
        if (!q.exec()) {
            QMessageBox::warning(this, "Ошибка БД", q.lastError().text());
            return;
        }
        while (q.next()) products.append({q.value(0).toInt(), q.value(1).toString()});
    }

    // lines
    {
        QSqlQuery q(db);
        q.prepare("SELECT product_id, qty_kg, price, line_sum FROM document_lines WHERE document_id = :id ORDER BY id");
        q.bindValue(":id", m_docId);

        if (!q.exec()) {
            QMessageBox::warning(this, "Ошибка БД", q.lastError().text());
            return;
        }

        m_linesTable->blockSignals(true);

        while (q.next()) {
            const int row = m_linesTable->rowCount();
            m_linesTable->insertRow(row);

            const int productId = q.value(0).toInt();
            const double qty = q.value(1).toDouble();
            const Decimal price = decimalFromVariant(q.value(2));

            auto* combo = new QComboBox(this);
            for (auto& p : products) combo->addItem(p.second, p.first);

            int idx = combo->findData(productId);
            if (idx >= 0) {
                combo->setCurrentIndex(idx);
            } else {
                // если товара вообще нет в products (очень редкий случай) — добавим как “не найден”
                combo->addItem(QString("[не найден ID %1]").arg(productId), productId);
                combo->setCurrentIndex(combo->count() - 1);
            }

            m_linesTable->setCellWidget(row, 0, combo);

            auto* qtyItem = new QTableWidgetItem(QString::number(qty, 'f', 3));
            qtyItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_linesTable->setItem(row, 1, qtyItem);

            auto* priceItem = new QTableWidgetItem(money(price));
            priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_linesTable->setItem(row, 2, priceItem);

            auto* sumItem = new QTableWidgetItem("0.00");
            sumItem->setFlags(sumItem->flags() & ~Qt::ItemIsEditable);
            sumItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_linesTable->setItem(row, 3, sumItem);

            connect(combo, &QComboBox::currentIndexChanged, this, [this, row]() {
                QSqlDatabase db = DbManager::instance().database();
                if (!db.isOpen()) return;
                auto* combo = qobject_cast<QComboBox*>(m_linesTable->cellWidget(row, 0));
                if (!combo) return;

                QSqlQuery qp(db);
                qp.prepare("SELECT price FROM products WHERE id = :id");
                qp.bindValue(":id", combo->currentData());
                if (qp.exec() && qp.next()) {
                    const Decimal price = decimalFromVariant(qp.value(0));
                    m_linesTable->blockSignals(true);
                    m_linesTable->item(row, 2)->setText(money(price));
                    m_linesTable->blockSignals(false);
                    onRecalcTotals();
                }
            });
        }

        m_linesTable->blockSignals(false);
    }

    onRecalcTotals();
}

bool TtnForm::validateForm()
{
    const QString number = m_numberEdit->text().trimmed();
    if (number.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите номер ТТН");
        return false;
    }

    // Проверка уникальности номера (для активных, не скрытых документов).
    // В схеме действует UNIQUE(number, doc_type), поэтому дублирование приведёт к ошибке SQLite.
    // Лучше перехватить это на уровне формы и показать понятное сообщение.
    {
        QSqlDatabase db = DbManager::instance().database();
        if (!db.isOpen()) {
            QMessageBox::critical(this, "Ошибка", "База данных не открыта");
            return false;
        }

        QSqlQuery q(db);
        q.prepare("SELECT id FROM documents "
                  "WHERE doc_type='transfer' AND number = :num AND is_deleted = 0 AND id <> :id "
                  "LIMIT 1");
        q.bindValue(":num", number);
        q.bindValue(":id", m_docId);
        if (!q.exec()) {
            QMessageBox::warning(this, "Ошибка БД", q.lastError().text());
            return false;
        }
        if (q.next()) {
            QMessageBox::warning(this, "Ошибка", "ТТН с таким номером уже существует");
            return false;
        }
    }

    if (m_senderCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите отправителя");
        return false;
    }

    if (m_receiverCombo->currentIndex() < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите получателя");
        return false;
    }

    if (m_linesTable->rowCount() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Добавьте хотя бы одну строку товара");
        return false;
    }

    for (int r = 0; r < m_linesTable->rowCount(); ++r) {
        const double qty = toDoubleSafe(m_linesTable->item(r, 1)->text());
        if (qty <= 0.0) {
            QMessageBox::warning(this, "Ошибка", "Количество (кг) должно быть > 0");
            return false;
        }
    }

    return true;
}

bool TtnForm::insertDocument(int& outDocId)
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("INSERT INTO documents (doc_type, number, date, status, sender_id, receiver_id, total_amount, notes) "
              "VALUES ('transfer', :number, :date, 'DRAFT', :sender, :receiver, :total, :notes)");

    q.bindValue(":number", m_numberEdit->text().trimmed());
    q.bindValue(":date", m_dateEdit->date().toString("yyyy-MM-dd"));
    q.bindValue(":sender", m_senderCombo->currentData().toInt());
    q.bindValue(":receiver", m_receiverCombo->currentData().toInt());
    q.bindValue(":total", decimalToString(Decimal(0)));
    q.bindValue(":notes", m_notesEdit->toPlainText().trimmed());

    if (!q.exec()) {
        QMessageBox::critical(this, "Ошибка БД", q.lastError().text());
        return false;
    }

    outDocId = q.lastInsertId().toInt();
    return outDocId > 0;
}

bool TtnForm::updateDocument()
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("UPDATE documents SET number=:number, date=:date, sender_id=:sender, receiver_id=:receiver, notes=:notes, "
              "updated_at=datetime('now') WHERE id=:id AND doc_type='transfer'");

    q.bindValue(":number", m_numberEdit->text().trimmed());
    q.bindValue(":date", m_dateEdit->date().toString("yyyy-MM-dd"));
    q.bindValue(":sender", m_senderCombo->currentData().toInt());
    q.bindValue(":receiver", m_receiverCombo->currentData().toInt());
    q.bindValue(":notes", m_notesEdit->toPlainText().trimmed());
    q.bindValue(":id", m_docId);

    if (!q.exec()) {
        QMessageBox::critical(this, "Ошибка БД", q.lastError().text());
        return false;
    }

    return true;
}

bool TtnForm::saveLines(int docId)
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) return false;

    {
        QSqlQuery del(db);
        del.prepare("DELETE FROM document_lines WHERE document_id = :id");
        del.bindValue(":id", docId);
        if (!del.exec()) {
            QMessageBox::critical(this, "Ошибка БД", del.lastError().text());
            return false;
        }
    }

    QSqlQuery ins(db);
    ins.prepare("INSERT INTO document_lines (document_id, product_id, qty_kg, price, line_sum) "
                "VALUES (:doc, :prod, :qty, :price, :sum)");

    for (int r = 0; r < m_linesTable->rowCount(); ++r) {
        auto* combo = qobject_cast<QComboBox*>(m_linesTable->cellWidget(r, 0));
        if (!combo) continue;

        const int productId = combo->currentData().toInt();
        const double qty = toDoubleSafe(m_linesTable->item(r, 1)->text());
        const Decimal price = toDecimalSafe(m_linesTable->item(r, 2)->text());
        const Decimal sum = price * Decimal(qty);

        ins.bindValue(":doc", docId);
        ins.bindValue(":prod", productId);
        ins.bindValue(":qty", qty);
        ins.bindValue(":price", decimalToString(price));
        ins.bindValue(":sum", decimalToString(sum));

        if (!ins.exec()) {
            QMessageBox::critical(this, "Ошибка БД", ins.lastError().text());
            return false;
        }
    }

    return true;
}

bool TtnForm::recalcAndSaveTotal(int docId)
{
    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) return false;

    QSqlQuery q(db);
    q.prepare("SELECT COALESCE(SUM(line_sum), 0) FROM document_lines WHERE document_id = :id");
    q.bindValue(":id", docId);
    if (!q.exec() || !q.next()) {
        QMessageBox::warning(this, "Ошибка БД", q.lastError().text());
        return false;
    }

    const Decimal total = decimalFromVariant(q.value(0));

    QSqlQuery upd(db);
    upd.prepare("UPDATE documents SET total_amount = :t, updated_at=datetime('now') WHERE id = :id");
    upd.bindValue(":t", decimalToString(total));
    upd.bindValue(":id", docId);

    if (!upd.exec()) {
        QMessageBox::warning(this, "Ошибка БД", upd.lastError().text());
        return false;
    }

    return true;
}

void TtnForm::onSaveClicked()
{
    if (!validateForm())
        return;

    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return;
    }

    if (!db.transaction()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось начать транзакцию");
        return;
    }

    int docId = m_docId;

    bool ok = true;
    if (docId <= 0) ok = insertDocument(docId);
    else ok = updateDocument();

    if (ok) ok = saveLines(docId);
    if (ok) ok = recalcAndSaveTotal(docId);

    if (!ok) {
        db.rollback();
        return;
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::warning(this, "Ошибка", "Не удалось зафиксировать изменения");
        return;
    }

    m_docId = docId;

    emit saved();
    accept();
}

void TtnForm::onCancelClicked()
{
    reject();
}
