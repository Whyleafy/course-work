#ifndef TTNWIDGET_H
#define TTNWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlTableModel>

class TTNWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TTNWidget(QWidget *parent = nullptr);

private slots:
    void onAddClicked();
    void onEditClicked();
    void onPostClicked();
    void onCancelClicked();
    void onDeleteClicked();
    void onRefreshClicked();
    void onSelectionChanged();

private:
    void setupUi();
    void refreshModel();

    int selectedDocId() const;
    QString selectedDocStatus() const;

    void updateButtonsByStatus();

    bool postDocumentSql(int documentId);
    bool cancelDocumentSql(int documentId);

    bool deleteTtnSql(int documentId);

    double stockBalanceForProduct(int productId);
    bool documentIsTransferDraftOrPosted(int documentId, QString* outStatus);

private:
    QTableView* m_tableView = nullptr;
    QSqlTableModel* m_model = nullptr;

    QPushButton* m_addButton = nullptr;
    QPushButton* m_editButton = nullptr;
    QPushButton* m_postButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QPushButton* m_refreshButton = nullptr;
};

#endif // TTNWIDGET_H
