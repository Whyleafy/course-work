#ifndef COUNTERPARTIESWIDGET_H
#define COUNTERPARTIESWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlTableModel>

class CounterpartiesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CounterpartiesWidget(QWidget *parent = nullptr);

private slots:
    void onAddClicked();
    void onEditClicked();
    void onRefreshClicked();

private:
    void setupUi();
    void refreshModel();
    
    QTableView* m_tableView;
    QSqlTableModel* m_model;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_refreshButton;
};

#endif // COUNTERPARTIESWIDGET_H

