cmake_minimum_required(VERSION 3.20)

project(WholesaleTradeApp
    VERSION 1.0.0
    LANGUAGES CXX
)

# ----------------------------------------
# C++ настройки
# ----------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----------------------------------------
# macOS Apple Silicon
# ----------------------------------------
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)
endif()

# ----------------------------------------
# Qt
# ----------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Sql
    Pdf
)

# ----------------------------------------
# Sources
# ----------------------------------------
set(SOURCES
    src/main.cpp
    ui/MainWindow.cpp
    src/DbManager.cpp
    src/MigrationRunner.cpp
    src/repositories/ProductRepository.cpp
    src/repositories/DocumentHelpers.cpp
    src/repositories/DocumentRepository.cpp
    src/repositories/DocumentLineRepository.cpp
    src/repositories/StockRepository.cpp
    ui/CounterpartyForm.cpp
    src/DocumentService.cpp
    ui/widgets/ProductsWidget.cpp
    ui/widgets/CounterpartiesWidget.cpp
    ui/ProductForm.cpp
    ui/widgets/SupplyWidget.cpp
    ui/TTNForm.cpp
    ui/SupplyForm.cpp
    ui/WriteOffForm.cpp
    ui/widgets/TTNWidget.cpp
    ui/widgets/StockBalancesWidget.cpp
    ui/widgets/MovementsWidget.cpp

    # 🔥 ВАЖНО — ресурсы должны быть ТУТ
    resources.qrc
)

set(HEADERS
    ui/MainWindow.h
    include/DbManager.h
    include/MigrationRunner.h
    include/repositories/IProductRepository.h
    include/repositories/IDocumentRepository.h
    include/repositories/IStockRepository.h
    include/repositories/ProductRepository.h
    include/repositories/IDocumentLineRepository.h
    include/DocumentService.h
    ui/CounterpartyForm.h
    ui/WriteOffForm.h
    ui/widgets/ProductsWidget.h
    ui/widgets/CounterpartiesWidget.h
    ui/widgets/TTNWidget.h
    ui/ProductForm.h
    ui/SupplyForm.h
    ui/widgets/SupplyWidget.h
    ui/TTNForm.h
    ui/widgets/StockBalancesWidget.h
    ui/widgets/MovementsWidget.h
)

# ----------------------------------------
# Executable
# ----------------------------------------
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

# ----------------------------------------
# Qt linking
# ----------------------------------------
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Pdf
)

# ----------------------------------------
# Include directories
# ----------------------------------------
target_include_directories(${PROJECT_NAME}
    PRIVATE
        include
        ui
)

# ----------------------------------------
# macOS bundle
# ----------------------------------------
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST
            "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME
            "${PROJECT_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER
            "com.wholesaletrade.app"
        MACOSX_BUNDLE_BUNDLE_VERSION
            "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING
            "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    )
endif()
