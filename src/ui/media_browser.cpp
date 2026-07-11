#include "media_browser.h"
#include "project.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QAction>

MediaBrowser::MediaBrowser(Project* project, QWidget* parent)
    : QWidget(parent), m_project(project) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* header = new QWidget(this);
    auto* hLayout = new QHBoxLayout(header);
    hLayout->setContentsMargins(8, 4, 8, 4);
    auto* label = new QLabel("Media Browser", this);
    label->setStyleSheet("font-weight: bold; color: #ccc; font-size: 11px;");
    hLayout->addWidget(label);
    hLayout->addStretch();
    layout->addWidget(header);

    m_list = new QListWidget(this);
    m_list->setAlternatingRowColors(true);
    m_list->setStyleSheet(
        "QListWidget { background: #222; border: none; color: #ddd; }"
        "QListWidget::item { padding: 4px 8px; }"
        "QListWidget::item:selected { background: #2a5080; }"
        "QListWidget::item:alternate { background: #282828; }");
    layout->addWidget(m_list);

    // Context menu
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_list, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto* item = m_list->itemAt(pos);
        if (!item) return;
        QMenu menu(this);
        menu.addAction("Preview", this, [this, item]() {
            emit sourcePreviewRequested(item->data(Qt::UserRole).toString());
        });
        menu.addAction("Add to Timeline", this, [this, item]() {
            emit clipAddedToTimeline(item->data(Qt::UserRole).toString());
        });
        menu.exec(m_list->mapToGlobal(pos));
    });

    // Single click → source preview
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        emit sourcePreviewRequested(item->data(Qt::UserRole).toString());
    });

    // Double-click → add to timeline
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        emit clipAddedToTimeline(item->data(Qt::UserRole).toString());
    });

    if (m_project) {
        connect(m_project, &Project::assetAdded, this, [this](const QString&) { refresh(); });
        connect(m_project, &Project::assetRemoved, this, [this](const QString&) { refresh(); });
    }
    refresh();
}

void MediaBrowser::setProject(Project* project) {
    if (m_project) disconnect(m_project, nullptr, this, nullptr);
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::assetAdded, this, [this](const QString&) { refresh(); });
        connect(m_project, &Project::assetRemoved, this, [this](const QString&) { refresh(); });
    }
    refresh();
}

void MediaBrowser::refresh() {
    m_list->clear();
    if (!m_project) return;
    for (const auto& asset : m_project->assets()) {
        auto* item = new QListWidgetItem(asset->name);
        item->setData(Qt::UserRole, asset->id);
        item->setToolTip(asset->path);
        m_list->addItem(item);
    }
}
