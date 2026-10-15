#include "ProjectExplorerDock.h"
#include "Theme.h"
#include "../managers/ProjectController.h"

#include <QFont>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

namespace { constexpr int kPathRole = Qt::UserRole + 1; }

ProjectExplorerDock::ProjectExplorerDock(ProjectController* project, QWidget* parent)
    : QDockWidget("Project", parent), m_project(project) {
    setObjectName("ProjectExplorerDock");   // needed to save/restore the layout

    auto* body = new QWidget();
    auto* v = new QVBoxLayout(body);
    v->setContentsMargins(4, 4, 4, 4);

    m_header = new QLabel("No project open");
    Theme::setRole(m_header, "title");
    v->addWidget(m_header);

    auto* buttons = new QHBoxLayout();
    auto* add = new QPushButton("+ File");
    add->setToolTip("Add a source file or header to the project");
    auto* remove = new QPushButton("– Remove");
    Theme::setRole(remove, "subtle");
    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch();
    v->addLayout(buttons);

    m_model = new QStandardItemModel(this);
    m_tree = new QTreeView();
    m_tree->setModel(m_model);
    m_tree->setHeaderHidden(true);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    v->addWidget(m_tree, 1);
    setWidget(body);

    connect(add, &QPushButton::clicked, this, &ProjectExplorerDock::addFileRequested);
    connect(remove, &QPushButton::clicked, this, [this]() {
        const QString p = selectedPath();
        if (!p.isEmpty()) emit removeFileRequested(p);
    });
    connect(m_tree, &QTreeView::doubleClicked, this, [this](const QModelIndex& idx) {
        const QString p = idx.data(kPathRole).toString();
        if (!p.isEmpty()) emit openFileRequested(p);
    });
    connect(m_tree, &QTreeView::customContextMenuRequested, this, &ProjectExplorerDock::showContextMenu);

    connect(m_project, &ProjectController::projectChanged, this, &ProjectExplorerDock::refresh);
    connect(m_project, &ProjectController::fileDirtyChanged, this, [this]() { refresh(); });
    refresh();
}

QString ProjectExplorerDock::selectedPath() const {
    const QModelIndex idx = m_tree->currentIndex();
    return idx.isValid() ? idx.data(kPathRole).toString() : QString();
}

void ProjectExplorerDock::refresh() {
    // Keep the expanded folders and the selection across rebuilds.
    const QString selected = selectedPath();
    m_model->clear();
    if (!m_project->isOpen()) {
        m_header->setText("No project open");
        return;
    }
    const QString name = m_project->workspace().name();
    m_header->setText(m_project->isDirty() ? name + " *" : name);

    const QSet<QString> dirty = m_project->dirtyFiles();
    QMap<QString, QStandardItem*> folders;
    auto folderFor = [&](const QString& folder) -> QStandardItem* {
        if (folder.isEmpty()) return m_model->invisibleRootItem();
        if (folders.contains(folder)) return folders[folder];
        QStandardItem* parent = m_model->invisibleRootItem();
        QString acc;
        for (const QString& seg : folder.split('/')) {
            acc += (acc.isEmpty() ? "" : "/") + seg;
            if (!folders.contains(acc)) {
                auto* it = new QStandardItem(seg + "/");
                it->setEditable(false);
                parent->appendRow(it);
                folders[acc] = it;
            }
            parent = folders[acc];
        }
        return parent;
    };

    const QString activeMain = [&]() {
        const auto& cfg = m_project->workspace().configuration();
        const BuildTarget* t = m_project->workspace().findTarget(cfg.activeTargetName);
        return t ? t->mainSourceFile : QString();
    }();

    QStringList paths = m_project->filePaths();
    paths.sort();
    for (const QString& p : paths) {
        const int slash = p.lastIndexOf('/');
        const QString base = p.mid(slash + 1);
        auto* it = new QStandardItem((dirty.contains(p) ? QString("● ") : QString()) + base);
        it->setData(p, kPathRole);
        it->setEditable(false);
        const bool isHeader = m_project->workspace().findSourceFile(p).isHeader;
        QStringList tip = {p, isHeader ? "header" : "source"};
        if (dirty.contains(p)) tip << "unsaved changes";
        if (p == activeMain) tip << "main file of the active target";
        it->setToolTip(tip.join(" · "));
        QFont f = it->font();
        if (p == activeMain) f.setBold(true);
        if (isHeader) f.setItalic(true);
        it->setFont(f);
        folderFor(slash < 0 ? QString() : p.left(slash))->appendRow(it);
    }
    m_tree->expandAll();
    if (!selected.isEmpty()) {
        const auto matches = m_model->match(m_model->index(0, 0), kPathRole, selected, 1, Qt::MatchRecursive);
        if (!matches.isEmpty()) m_tree->setCurrentIndex(matches.first());
    }
}

void ProjectExplorerDock::showContextMenu(const QPoint& pos) {
    const QModelIndex idx = m_tree->indexAt(pos);
    QMenu menu(this);
    const QString path = idx.isValid() ? idx.data(kPathRole).toString() : QString();
    if (!path.isEmpty()) {
        menu.addAction("Open", this, [this, path]() { emit openFileRequested(path); });
        if (!m_project->workspace().findSourceFile(path).isHeader)
            menu.addAction("Use as main file of the target", this, [this, path]() { emit setMainRequested(path); });
        menu.addSeparator();
        menu.addAction("Remove from project", this, [this, path]() { emit removeFileRequested(path); });
        menu.addSeparator();
    }
    menu.addAction("Add file…", this, [this]() { emit addFileRequested(); });
    menu.exec(m_tree->viewport()->mapToGlobal(pos));
}
