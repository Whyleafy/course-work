#ifndef SUPPLYWIDGET_H
#define SUPPLYWIDGET_H

#include <QWidget>

class QTableView;
class QPushButton;
class QSqlTableModel;

class DocumentService;

class SupplyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SupplyWidget(DocumentService* docService, QWidget* parent = nullptr);

private slots:
    void onAddClicked();
    void onEditClicked();
    void onPostClicked();
    void onCancelClicked();
    void onRefreshClicked();

private:
    void setupUi();
    void refreshModel();
    int selectedDocId() const;

private:
    DocumentService* m_docService = nullptr;

    QTableView* m_tableView = nullptr;
    QSqlTableModel* m_model = nullptr;

    QPushButton* m_addButton = nullptr;
    QPushButton* m_editButton = nullptr;
    QPushButton* m_postButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_refreshButton = nullptr;
};

#endif // SUPPLYWIDGET_H
