// Tankoban 3 — MainWindow (Step 2 + Detail + Addons).
//
// The app shell: [ Sidebar | content QStackedWidget ]. Each nav id maps to a page in
// the stack. The Detail page is pushed over the shell (sidebar stays visible, Harbor-
// style) via openDetail(); Back returns to the prior route. The AddonRegistry is owned
// here and shared with the Addons page (and later the catalog/stream services).

#pragma once

#include <optional>

#include <QHash>
#include <QWidget>

#include "core/MetaDetail.h"
#include "core/MetaItem.h"

class QStackedWidget;
class QResizeEvent;

namespace tankoban {

class Sidebar;
class DetailPage;
class AddonRegistry;
class PlayPickerPage;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QWidget* makePlaceholder(const QString& title);
    void openDetail(const MetaItem& meta);
    void openPlayPicker(const MetaItem& meta, std::optional<EpisodeItem> episode);
    MetaDetail detailFromMetaItem(const MetaItem& meta) const;

    Sidebar* m_sidebar = nullptr;
    QStackedWidget* m_content = nullptr;
    QHash<QString, int> m_pageIndex;
    AddonRegistry* m_registry = nullptr;
    DetailPage* m_detailPage = nullptr;
    PlayPickerPage* m_playPicker = nullptr;
    int m_detailIndex = -1;
    int m_returnIndex = 0;
};

} // namespace tankoban
