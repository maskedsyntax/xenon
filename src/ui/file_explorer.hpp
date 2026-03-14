#pragma once

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QVBoxLayout>

namespace xenon::ui {

class FileExplorer : public QWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget* parent = nullptr);
    ~FileExplorer() override = default;

    void setRootPath(const QString& path);

signals:
    void fileActivated(const QString& path);

private:
    QTreeView* tree_view_;
    QFileSystemModel* model_;
};

} // namespace xenon::ui
