#ifndef PROJECTEXPLORERDOCK_H
#define PROJECTEXPLORERDOCK_H

#include <QDockWidget>
#include <QLabel>
#include <QStandardItemModel>
#include <QTreeView>

class ProjectController;

// File tree of the open project. Pure view: all state lives in ProjectController.
class ProjectExplorerDock : public QDockWidget {
    Q_OBJECT

public:
    explicit ProjectExplorerDock(ProjectController* project, QWidget* parent = nullptr);

    QTreeView* tree() const { return m_tree; }
    QStandardItemModel* model() const { return m_model; }
    // Path of the selected file ("" for folders / no selection)
    QString selectedPath() const;
    void refresh();

signals:
    void openFileRequested(const QString& path);
    void addFileRequested();
    void removeFileRequested(const QString& path);
    void setMainRequested(const QString& path);

private:
    void showContextMenu(const QPoint& pos);

    ProjectController* m_project;
    QTreeView* m_tree;
    QStandardItemModel* m_model;
    QLabel* m_header;
};

#endif // PROJECTEXPLORERDOCK_H
