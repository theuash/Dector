#include "media_browser.h"
#include "project.h"
#include <QVBoxLayout>
#include <QLabel>

MediaBrowser::MediaBrowser(Project* project, QWidget* parent)
    : QWidget(parent), m_project(project) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    auto* label = new QLabel("Media Browser", this);
    label->setStyleSheet("font-weight: bold; padding: 4px;");
    layout->addWidget(label);

    m_list = new QListWidget(this);
    layout->addWidget(m_list);

    if (m_project) {
        connect(m_project, &Project::assetAdded, this, [this](const QString&) { refresh(); });
        connect(m_project, &Project::assetRemoved, this, [this](const QString&) { refresh(); });
    }
    refresh();
}

void MediaBrowser::setProject(Project* project) {
    if (m_project) {
        disconnect(m_project, nullptr, this, nullptr);
    }
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
