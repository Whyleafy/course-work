#ifndef MOVEMENTSWIDGET_H
#define MOVEMENTSWIDGET_H

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QSqlQueryModel>

class MovementsModel : public QSqlQueryModel
{
    Q_OBJECT
public:
    explicit MovementsModel(QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    void setOnlyPosted(bool v);
    void setShowStorno(bool v);

    void refresh();

private:
    bool m_onlyPosted = false;
    bool m_showStorno = true;
};

class MovementsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MovementsWidget(QWidget *parent = nullptr);

private slots:
    void onRefreshClicked();
    void onOnlyPostedChanged(Qt::CheckState state);
    void onShowStornoChanged(Qt::CheckState state);

private:
    void setupUi();

    QTableView* m_tableView = nullptr;
    MovementsModel* m_model = nullptr;

    QCheckBox* m_onlyPostedCheck = nullptr;
    QCheckBox* m_showStornoCheck = nullptr;

    QPushButton* m_refreshButton = nullptr;
};

#endif // MOVEMENTSWIDGET_H
