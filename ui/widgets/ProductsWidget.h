#ifndef PRODUCTSWIDGET_H
#define PRODUCTSWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlTableModel>

class ProductsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProductsWidget(QWidget *parent = nullptr);

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

#endif // PRODUCTSWIDGET_H
