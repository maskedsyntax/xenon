#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

namespace xenon::ui {

class CommandPalette : public QDialog {
    Q_OBJECT

public:
    explicit CommandPalette(QWidget* parent = nullptr);
    ~CommandPalette() override = default;

    void showPalette();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void onItemSelected(QListWidgetItem* item);

private:
    QLineEdit* search_edit_;
    QListWidget* list_widget_;
};

} // namespace xenon::ui
