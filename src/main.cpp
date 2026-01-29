#include <gtkmm.h>
#include "ui/main_window.hpp"

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "org.xenon.Editor");

    xenon::ui::MainWindow window(app);

    return app->run(window);
}
