#include "MainWindow.h"

#include "widgets/ProductsWidget.h"
#include "widgets/CounterpartiesWidget.h"
#include "widgets/SupplyWidget.h"
#include "widgets/TTNWidget.h"
#include "widgets/StockBalancesWidget.h"
#include "widgets/MovementsWidget.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(DocumentService* docService, QWidget *parent)
    : QMainWindow(parent)
    , m_docService(docService)
{
    setupUi();
    createMenuBar();
    setWindowTitle("Система управления оптовой торговлей");
    resize(1200, 800);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);

    m_tabWidget->addTab(new ProductsWidget(this), "Товары");
    m_tabWidget->addTab(new CounterpartiesWidget(this), "Контрагенты");

    m_tabWidget->addTab(new SupplyWidget(m_docService, this), "Поставки");
    m_tabWidget->addTab(new TTNWidget(this), "ТТН");

    m_tabWidget->addTab(new StockBalancesWidget(this), "Остатки");
    m_tabWidget->addTab(new MovementsWidget(this), "Движения");
}

void MainWindow::createMenuBar()
{
    QMenuBar* menuBar = this->menuBar();

    QMenu* fileMenu = menuBar->addMenu("Файл");
    QAction* exitAction = fileMenu->addAction("Выход");
    connect(exitAction, &QAction::triggered, this, &MainWindow::onExitClicked);

    QMenu* helpMenu = menuBar->addMenu("Справка");
    QAction* aboutAction = helpMenu->addAction("О программе");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAboutClicked);
}

void MainWindow::onAboutClicked()
{
    QMessageBox::about(this, "О программе",
        "Система управления оптовой торговлей\n"
        "Версия 1.0");
}

void MainWindow::onExitClicked()
{
    QApplication::quit();
}
