#include <QApplication>
#include "ui/main_window.hpp"
#include "ui/style_manager.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Xenon");
    app.setOrganizationName("MaskedSyntax");

    app.setStyleSheet(xenon::ui::StyleManager::getDarkStyle());

    xenon::ui::MainWindow window;
    window.show();

    return app.exec();
}
