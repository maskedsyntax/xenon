#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>

namespace xenon::ui {

class FindReplaceWidget : public QWidget {
    Q_OBJECT

public:
    explicit FindReplaceWidget(QWidget* parent = nullptr);

    void showFind();
    void showReplace();
    
    QString findText() const { return find_edit_->text(); }
    QString replaceText() const { return replace_edit_->text(); }
    bool isCaseSensitive() const { return case_check_->isChecked(); }
    bool isRegex() const { return regex_check_->isChecked(); }

signals:
    void findNext();
    void findPrevious();
    void replace();
    void replaceAll();
    void closeRequested();

private:
    QLineEdit* find_edit_;
    QLineEdit* replace_edit_;
    QCheckBox* case_check_;
    QCheckBox* regex_check_;
    
    QWidget* replace_container_;
};

} // namespace xenon::ui
