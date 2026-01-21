#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>

class DocumentService;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(DocumentService* docService, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAboutClicked();
    void onExitClicked();

private:
    void setupUi();
    void createMenuBar();

private:
    DocumentService* m_docService = nullptr;
    QTabWidget* m_tabWidget = nullptr;
};

#endif // MAINWINDOW_H
