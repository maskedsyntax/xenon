#include <gtkmm.h>
#include "ui/main_window.hpp"

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.xenon.Editor");

    app->signal_startup().connect([app]() {
        auto window = new xenon::ui::MainWindow(app);
        window->present();
    });

    return app->run();
}
