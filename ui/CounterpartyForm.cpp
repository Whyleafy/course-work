#include "CounterpartyForm.h"
#include "DbManager.h"

#include <QMessageBox>
#include <QRegularExpression>
#include <QSignalBlocker>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>


#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QSqlDatabase>

static QString digitsOnly(QString s)
{
    s.remove(QRegularExpression("[^0-9]"));
    return s;
}

static void normalizeLineEditDigits(QLineEdit* edit, int maxLen = -1)
{
    QObject::connect(edit, &QLineEdit::textChanged, edit, [edit, maxLen](const QString& t) {
        QString cleaned = digitsOnly(t);
        if (maxLen > 0)
            cleaned = cleaned.left(maxLen);

        if (cleaned == t)
            return;

        QSignalBlocker blocker(edit);
        edit->setText(cleaned);
        edit->setCursorPosition(cleaned.size());
    });
}

static QVariant bindNullableText(const QString& s)
{
    const QString t = s.trimmed();
    return t.isEmpty() ? QVariant() : QVariant(t);
}

static QVariant bindNullableDigits(const QString& s)
{
    const QString t = s.trimmed();
    return t.isEmpty() ? QVariant() : QVariant(t);
}

static bool allReqEmpty(const QString& inn, const QString& kpp, const QString& bik,
                        const QString& bank, const QString& rAcc, const QString& kAcc)
{
    return inn.isEmpty() && kpp.isEmpty() && bik.isEmpty() &&
           bank.trimmed().isEmpty() && rAcc.isEmpty() && kAcc.isEmpty();
}

CounterpartyForm::CounterpartyForm(QWidget *parent)
    : QDialog(parent)
    , m_counterpartyId(0)
{
    setupUi();
    setupValidators();
}

void CounterpartyForm::setupUi()
{
    setWindowTitle("Контрагент");
    setModal(true);
    resize(500, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QGroupBox* mainGroup = new QGroupBox("Основные данные", this);
    QFormLayout* mainForm = new QFormLayout(mainGroup);

    m_nameEdit = new QLineEdit(this);
    mainForm->addRow("Наименование:", m_nameEdit);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("Поставщик", "supplier");
    m_typeCombo->addItem("Покупатель", "customer");
    m_typeCombo->addItem("Оба", "both");
    mainForm->addRow("Тип:", m_typeCombo);

    m_addressEdit = new QTextEdit(this);
    m_addressEdit->setMaximumHeight(80);
    mainForm->addRow("Адрес:", m_addressEdit);

    mainLayout->addWidget(mainGroup);

    QGroupBox* reqGroup = new QGroupBox("Реквизиты", this);
    QFormLayout* reqForm = new QFormLayout(reqGroup);

    m_innEdit = new QLineEdit(this);
    reqForm->addRow("ИНН:", m_innEdit);

    m_kppEdit = new QLineEdit(this);
    reqForm->addRow("КПП:", m_kppEdit);

    m_bikEdit = new QLineEdit(this);
    reqForm->addRow("БИК:", m_bikEdit);

    m_bankNameEdit = new QLineEdit(this);
    reqForm->addRow("Банк:", m_bankNameEdit);

    m_rAccountEdit = new QLineEdit(this);
    reqForm->addRow("Расчетный счет:", m_rAccountEdit);

    m_kAccountEdit = new QLineEdit(this);
    reqForm->addRow("Корреспондентский счет:", m_kAccountEdit);

    mainLayout->addWidget(reqGroup);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_saveButton = new QPushButton("Сохранить", this);
    m_cancelButton = new QPushButton("Отмена", this);

    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(m_saveButton, &QPushButton::clicked, this, &CounterpartyForm::onSaveClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &CounterpartyForm::onCancelClicked);
}

void CounterpartyForm::setupValidators()
{

    normalizeLineEditDigits(m_innEdit, 12);
    normalizeLineEditDigits(m_kppEdit, 9);
    normalizeLineEditDigits(m_bikEdit, 9);
    normalizeLineEditDigits(m_rAccountEdit, 20);
    normalizeLineEditDigits(m_kAccountEdit, 20);

    m_innEdit->setPlaceholderText("10 или 12 цифр");
    m_kppEdit->setPlaceholderText("9 цифр");
    m_bikEdit->setPlaceholderText("9 цифр");
    m_rAccountEdit->setPlaceholderText("20 цифр");
    m_kAccountEdit->setPlaceholderText("20 цифр");
}

void CounterpartyForm::loadData(int counterpartyId)
{
    m_counterpartyId = counterpartyId;

    if (m_counterpartyId <= 0) {
        setWindowTitle("Добавить контрагента");

        m_nameEdit->clear();
        m_typeCombo->setCurrentIndex(0);
        m_addressEdit->clear();

        m_innEdit->clear();
        m_kppEdit->clear();
        m_bikEdit->clear();
        m_bankNameEdit->clear();
        m_rAccountEdit->clear();
        m_kAccountEdit->clear();
        return;
    }

    setWindowTitle(QString("Редактировать контрагента (ID %1)").arg(m_counterpartyId));

    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return;
    }

    // counterparties
    {
        QSqlQuery q(db);
        q.prepare("SELECT name, type, address FROM counterparties WHERE id = :id");
        q.bindValue(":id", m_counterpartyId);

        if (!q.exec()) {
            QMessageBox::critical(this, "Ошибка БД",
                                  "Не удалось загрузить контрагента:\n" + q.lastError().text());
            return;
        }
        if (!q.next()) {
            QMessageBox::warning(this, "Не найдено", "Контрагент не найден");
            return;
        }

        m_nameEdit->setText(q.value(0).toString());

        const QString type = q.value(1).toString();
        int idx = m_typeCombo->findData(type);
        if (idx < 0) idx = 0;
        m_typeCombo->setCurrentIndex(idx);

        m_addressEdit->setPlainText(q.value(2).toString());
    }

    // requisites
    {
        QSqlQuery q(db);
        q.prepare("SELECT inn, kpp, bik, bank_name, r_account, k_account "
                  "FROM requisites WHERE counterparty_id = :id LIMIT 1");
        q.bindValue(":id", m_counterpartyId);

        if (!q.exec()) {
            QMessageBox::warning(this, "Ошибка БД",
                                 "Не удалось загрузить реквизиты:\n" + q.lastError().text());
        } else if (q.next()) {
            m_innEdit->setText(digitsOnly(q.value(0).toString()));
            m_kppEdit->setText(digitsOnly(q.value(1).toString()));
            m_bikEdit->setText(digitsOnly(q.value(2).toString()));
            m_bankNameEdit->setText(q.value(3).toString());
            m_rAccountEdit->setText(digitsOnly(q.value(4).toString()));
            m_kAccountEdit->setText(digitsOnly(q.value(5).toString()));
        } else {
            m_innEdit->clear();
            m_kppEdit->clear();
            m_bikEdit->clear();
            m_bankNameEdit->clear();
            m_rAccountEdit->clear();
            m_kAccountEdit->clear();
        }
    }
}

bool CounterpartyForm::validateForm()
{
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите наименование контрагента");
        return false;
    }

    const QString inn  = digitsOnly(m_innEdit->text());
    const QString kpp  = digitsOnly(m_kppEdit->text());
    const QString bik  = digitsOnly(m_bikEdit->text());
    const QString rAcc = digitsOnly(m_rAccountEdit->text());
    const QString kAcc = digitsOnly(m_kAccountEdit->text());

    if (m_innEdit->text() != inn) m_innEdit->setText(inn);
    if (m_kppEdit->text() != kpp) m_kppEdit->setText(kpp);
    if (m_bikEdit->text() != bik) m_bikEdit->setText(bik);
    if (m_rAccountEdit->text() != rAcc) m_rAccountEdit->setText(rAcc);
    if (m_kAccountEdit->text() != kAcc) m_kAccountEdit->setText(kAcc);

    // ИНН: 10 или 12
    if (!inn.isEmpty() && !(inn.size() == 10 || inn.size() == 12)) {
        QMessageBox::warning(this, "Ошибка", "ИНН должен содержать 10 или 12 цифр");
        return false;
    }
    // КПП: 9
    if (!kpp.isEmpty() && kpp.size() != 9) {
        QMessageBox::warning(this, "Ошибка", "КПП должен содержать 9 цифр");
        return false;
    }
    // БИК: 9
    if (!bik.isEmpty() && bik.size() != 9) {
        QMessageBox::warning(this, "Ошибка", "БИК должен содержать 9 цифр");
        return false;
    }
    // Счета: 20
    if (!rAcc.isEmpty() && rAcc.size() != 20) {
        QMessageBox::warning(this, "Ошибка", "Расчетный счет должен содержать 20 цифр");
        return false;
    }
    if (!kAcc.isEmpty() && kAcc.size() != 20) {
        QMessageBox::warning(this, "Ошибка", "Корреспондентский счет должен содержать 20 цифр");
        return false;
    }

    return true;
}

void CounterpartyForm::onSaveClicked()
{
    if (!validateForm()) {
        return;
    }

    QSqlDatabase db = DbManager::instance().database();
    if (!db.isOpen()) {
        QMessageBox::critical(this, "Ошибка", "База данных не открыта");
        return;
    }

    const QString name = m_nameEdit->text().trimmed();
    const QString type = m_typeCombo->currentData().toString();
    const QString address = m_addressEdit->toPlainText().trimmed();

    const QString inn  = digitsOnly(m_innEdit->text());
    const QString kpp  = digitsOnly(m_kppEdit->text());
    const QString bik  = digitsOnly(m_bikEdit->text());
    const QString bankName = m_bankNameEdit->text().trimmed();
    const QString rAcc = digitsOnly(m_rAccountEdit->text());
    const QString kAcc = digitsOnly(m_kAccountEdit->text());

    if (!db.transaction()) {
        QMessageBox::warning(this, "Ошибка БД", "Не удалось начать транзакцию:\n" + db.lastError().text());
        return;
    }

    int id = m_counterpartyId;

    // 1) INSERT/UPDATE counterparties
    if (id <= 0) {
        QSqlQuery q(db);
        q.prepare("INSERT INTO counterparties (name, type, address, is_active, created_at, updated_at) "
                  "VALUES (:name, :type, :address, 1, datetime('now'), datetime('now'))");
        q.bindValue(":name", name);
        q.bindValue(":type", type);
        q.bindValue(":address", bindNullableText(address));

        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось добавить контрагента:\n" + q.lastError().text());
            return;
        }

        id = q.lastInsertId().toInt();
        if (id <= 0) {
            QSqlQuery q2(db);
            q2.exec("SELECT last_insert_rowid()");
            if (q2.next()) id = q2.value(0).toInt();
        }
        if (id <= 0) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось получить ID нового контрагента");
            return;
        }

        m_counterpartyId = id;
    } else {
        QSqlQuery q(db);
        q.prepare("UPDATE counterparties "
                  "SET name = :name, type = :type, address = :address, updated_at = datetime('now') "
                  "WHERE id = :id");
        q.bindValue(":name", name);
        q.bindValue(":type", type);
        q.bindValue(":address", bindNullableText(address));
        q.bindValue(":id", id);

        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось обновить контрагента:\n" + q.lastError().text());
            return;
        }
    }

    // 2) requisites (UPSERT или DELETE если пусто)
    if (allReqEmpty(inn, kpp, bik, bankName, rAcc, kAcc)) {
        QSqlQuery q(db);
        q.prepare("DELETE FROM requisites WHERE counterparty_id = :id");
        q.bindValue(":id", id);

        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось удалить реквизиты:\n" + q.lastError().text());
            return;
        }
    } else {
        QSqlQuery q(db);
        q.prepare(
            "INSERT INTO requisites (counterparty_id, inn, kpp, bik, bank_name, r_account, k_account, created_at, updated_at) "
            "VALUES (:id, :inn, :kpp, :bik, :bank, :racc, :kacc, datetime('now'), datetime('now')) "
            "ON CONFLICT(counterparty_id) DO UPDATE SET "
            "inn = excluded.inn, "
            "kpp = excluded.kpp, "
            "bik = excluded.bik, "
            "bank_name = excluded.bank_name, "
            "r_account = excluded.r_account, "
            "k_account = excluded.k_account, "
            "updated_at = datetime('now')"
        );

        q.bindValue(":id", id);
        q.bindValue(":inn",  bindNullableDigits(inn));
        q.bindValue(":kpp",  bindNullableDigits(kpp));
        q.bindValue(":bik",  bindNullableDigits(bik));
        q.bindValue(":bank", bindNullableText(bankName));
        q.bindValue(":racc", bindNullableDigits(rAcc));
        q.bindValue(":kacc", bindNullableDigits(kAcc));

        if (!q.exec()) {
            db.rollback();
            QMessageBox::critical(this, "Ошибка БД", "Не удалось сохранить реквизиты:\n" + q.lastError().text());
            return;
        }
    }

    if (!db.commit()) {
        db.rollback();
        QMessageBox::critical(this, "Ошибка БД", "Не удалось зафиксировать изменения:\n" + db.lastError().text());
        return;
    }

    emit saved();
    accept();
}

void CounterpartyForm::onCancelClicked()
{
    reject();
}
