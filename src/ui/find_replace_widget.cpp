#include "ui/find_replace_widget.hpp"
#include <QShortcut>

namespace xenon::ui {

FindReplaceWidget::FindReplaceWidget(QWidget* parent)
    : QWidget(parent) {
    
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(10, 5, 10, 5);
    main_layout->setSpacing(5);

    // Find Row
    auto* find_row = new QHBoxLayout();
    find_edit_ = new QLineEdit(this);
    find_edit_->setPlaceholderText("Find");
    find_edit_->setStyleSheet("background-color: #3c3c3c; color: white; border: 1px solid #555555; padding: 2px;");
    
    auto* next_btn = new QPushButton("Next", this);
    auto* prev_btn = new QPushButton("Prev", this);
    auto* close_btn = new QPushButton("X", this);
    close_btn->setFixedWidth(20);

    find_row->addWidget(find_edit_);
    find_row->addWidget(prev_btn);
    find_row->addWidget(next_btn);
    find_row->addWidget(close_btn);
    main_layout->addLayout(find_row);

    // Replace Row
    replace_container_ = new QWidget(this);
    auto* replace_layout = new QHBoxLayout(replace_container_);
    replace_layout->setContentsMargins(0, 0, 0, 0);
    replace_edit_ = new QLineEdit(this);
    replace_edit_->setPlaceholderText("Replace");
    replace_edit_->setStyleSheet("background-color: #3c3c3c; color: white; border: 1px solid #555555; padding: 2px;");
    
    auto* replace_btn = new QPushButton("Replace", this);
    auto* replace_all_btn = new QPushButton("All", this);

    replace_layout->addWidget(replace_edit_);
    replace_layout->addWidget(replace_btn);
    replace_layout->addWidget(replace_all_btn);
    main_layout->addWidget(replace_container_);

    // Options Row
    auto* options_row = new QHBoxLayout();
    case_check_ = new QCheckBox("Aa", this);
    case_check_->setToolTip("Case Sensitive");
    regex_check_ = new QCheckBox(".*", this);
    regex_check_->setToolTip("Use Regular Expression");
    
    options_row->addWidget(case_check_);
    options_row->addWidget(regex_check_);
    options_row->addStretch();
    main_layout->addLayout(options_row);

    setStyleSheet("background-color: #252526; border: 1px solid #3c3c3c; color: white;");

    connect(next_btn, &QPushButton::clicked, this, &FindReplaceWidget::findNext);
    connect(prev_btn, &QPushButton::clicked, this, &FindReplaceWidget::findPrevious);
    connect(replace_btn, &QPushButton::clicked, this, &FindReplaceWidget::replace);
    connect(replace_all_btn, &QPushButton::clicked, this, &FindReplaceWidget::replaceAll);
    connect(close_btn, &QPushButton::clicked, this, &FindReplaceWidget::closeRequested);
    
    connect(find_edit_, &QLineEdit::returnPressed, this, &FindReplaceWidget::findNext);
    connect(replace_edit_, &QLineEdit::returnPressed, this, &FindReplaceWidget::replace);

    auto* esc_shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(esc_shortcut, &QShortcut::activated, this, &FindReplaceWidget::closeRequested);
}

void FindReplaceWidget::showFind() {
    replace_container_->hide();
    show();
    find_edit_->setFocus();
    find_edit_->selectAll();
}

void FindReplaceWidget::showReplace() {
    replace_container_->show();
    show();
    find_edit_->setFocus();
    find_edit_->selectAll();
}

} // namespace xenon::ui
