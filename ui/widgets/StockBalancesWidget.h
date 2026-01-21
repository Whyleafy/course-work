#ifndef STOCKBALANCESWIDGET_H
#define STOCKBALANCESWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QString>
#include <QList>

class StockBalancesModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit StockBalancesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setShowInactive(bool v);
    void setHideZero(bool v);

    void refresh();

private:
    struct BalanceItem {
        int productId = 0;
        QString productName;
        int isActive = 1;
        QString unit;
        double price = 0.0;
        double balanceKg = 0.0;
    };

    QList<BalanceItem> m_data;

    bool m_showInactive = true;
    bool m_hideZero = false;
};

class StockBalancesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StockBalancesWidget(QWidget *parent = nullptr);

private slots:
    void onRefreshClicked();
    void onShowInactiveChanged(Qt::CheckState state);
    void onHideZeroChanged(Qt::CheckState state);

    void onWriteOffClicked();

private:
    void setupUi();

    int selectedProductId() const;

    QTableView* m_tableView = nullptr;
    StockBalancesModel* m_model = nullptr;

    QCheckBox* m_showInactiveCheck = nullptr;
    QCheckBox* m_hideZeroCheck = nullptr;

    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_writeOffButton = nullptr;
};

#endif // STOCKBALANCESWIDGET_H
