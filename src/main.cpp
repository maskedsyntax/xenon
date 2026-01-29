#include <gtkmm.h>
#include <gtksourceviewmm.h>
#include "ui/main_window.hpp"

int main(int argc, char* argv[]) {
    Gsv::init();
    auto app = Gtk::Application::create(argc, argv, "org.xenon.Editor");

    app->signal_startup().connect([app]() {
        // Initialize GtkSourceView
        auto lang_mgr = Gsv::LanguageManager::get_default();

        auto window = new xenon::ui::MainWindow(app);
        window->present();
    });

    return app->run();
}
