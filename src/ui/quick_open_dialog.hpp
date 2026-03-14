#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QStringList>

namespace xenon::ui {

class QuickOpenDialog : public QDialog {
    Q_OBJECT

public:
    explicit QuickOpenDialog(QWidget* parent = nullptr);
    ~QuickOpenDialog() override = default;

    void showDialog(const QString& rootPath);

signals:
    void fileSelected(const QString& path);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void onItemSelected(QListWidgetItem* item);

private:
    void refreshFiles(const QString& rootPath);

    QLineEdit* search_edit_;
    QListWidget* list_widget_;
    QStringList all_files_;
    QString root_path_;
};

} // namespace xenon::ui
