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
#include "core/StreamModels.h"

class QStackedWidget;
class QResizeEvent;
class QNetworkAccessManager;
class QPushButton;
class QEvent;
class QShowEvent;

namespace tankoban {

class Sidebar;
class DetailPage;
class AddonRegistry;
class PlayPickerPage;
class StreamService;
class VideoPlayerPage;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private:
    QWidget* makePlaceholder(const QString& title);
    void openDetail(const MetaItem& meta);
    void openPlayPicker(const MetaItem& meta, std::optional<EpisodeItem> episode);
    MetaDetail detailFromMetaItem(const MetaItem& meta) const;
    // Raw addon streams -> parsed -> scored -> ranked (M3/M4 brain), for the picker.
    RankedPicker buildPicker(const QVector<Stream>& streams) const;
    // Open the in-app player on a chosen DIRECT-link stream (no torrent/debrid this slice).
    void openDirectPlayer(const ScoredStream& stream);
    void closePlayer();   // leave the player page, restore chrome, return to the picker
    // Frameless Harbor-style window chrome (no OS title bar; min/max/close in our skin).
    void buildTopBar();
    void applyFramelessWin32Style();
    void updateMaxRestoreIcon();
    void toggleMaximizeRestore();

    QWidget* m_topBar = nullptr;
    QPushButton* m_btnMin = nullptr;
    QPushButton* m_btnMax = nullptr;
    QPushButton* m_btnClose = nullptr;
    Sidebar* m_sidebar = nullptr;
    QStackedWidget* m_content = nullptr;
    QHash<QString, int> m_pageIndex;
    AddonRegistry* m_registry = nullptr;
    DetailPage* m_detailPage = nullptr;
    PlayPickerPage* m_playPicker = nullptr;
    QNetworkAccessManager* m_streamNetwork = nullptr;
    StreamService* m_streamService = nullptr;
    VideoPlayerPage* m_player = nullptr;
    MetaDetail m_currentPickerMeta;
    std::optional<EpisodeItem> m_currentPickerEpisode;
    int m_detailIndex = -1;
    int m_playerIndex = -1;
    int m_returnIndex = 0;
    bool m_chromeApplied = false;
};

} // namespace tankoban
