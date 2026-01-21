#include "MainWindow.h"
#include "DbManager.h"
#include "MigrationRunner.h"
#include "DocumentService.h"

#include "repositories/DocumentRepository.h"
#include "repositories/DocumentLineRepository.h"
#include "repositories/StockRepository.h"
#include "repositories/ProductRepository.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    DbManager& dbManager = DbManager::instance();
    if (!dbManager.initialize()) {
        QMessageBox::critical(nullptr, "Ошибка",
            "Не удалось инициализировать базу данных.\n"
            "Приложение будет закрыто.");
        return 1;
    }

    MigrationRunner migrationRunner(dbManager.database());
    if (!migrationRunner.runMigrations()) {
        QMessageBox::critical(nullptr, "Ошибка",
            "Не удалось выполнить миграции базы данных.\n"
            "Приложение будет закрыто.");
        dbManager.close();
        return 1;
    }

    DocumentRepository docRepo(dbManager.database());
    DocumentLineRepository lineRepo(dbManager.database());
    StockRepository stockRepo(dbManager.database());
    ProductRepository productRepo(dbManager.database());

    DocumentService docService(&docRepo, &lineRepo, &stockRepo, &productRepo);

    MainWindow window(&docService);
    window.show();

    return app.exec();
}
