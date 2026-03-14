#include "ui/style_manager.hpp"

namespace xenon::ui {

QString StyleManager::getDarkStyle() {
    return R"QSS(
        QMainWindow { background-color: #1e1e1e; }
        QWidget { background-color: #1e1e1e; color: #d4d4d4; }
        
        QMenuBar { background-color: #2d2d2d; color: #d4d4d4; }
        QMenuBar::item:selected { background-color: #094771; }
        
        QMenu { background-color: #252526; border: 1px solid #454545; }
        QMenu::item:selected { background-color: #094771; }
        
        QScrollBar:vertical {
            background: #1e1e1e;
            width: 14px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #424242;
            min-height: 20px;
            border-radius: 7px;
            margin: 2px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        
        QScrollBar:horizontal {
            background: #1e1e1e;
            height: 14px;
            margin: 0px;
        }
        QScrollBar::handle:horizontal {
            background: #424242;
            min-width: 20px;
            border-radius: 7px;
            margin: 2px;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }

        QSplitter::handle { background-color: #3c3c3c; }
    )QSS";
}

} // namespace xenon::ui
