// Tankoban 3 — AddonsPage (Addons store).
//
// Harbor's Addons store shell: Discover / Browse / Installed tabs, a marketplace search,
// a paste-to-install bar, and an Adult toggle. Browse is the live stremio-addons.net
// marketplace (BrowsePane); Installed manages what's installed; Discover's featured
// landing is built next.

#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;

namespace tankoban {

class AddonRegistry;
class StremioAddonsProvider;
class BrowsePane;

class AddonsPage : public QWidget {
    Q_OBJECT
public:
    explicit AddonsPage(AddonRegistry* registry, QWidget* parent = nullptr);

private:
    void showTab(int idx);
    void refreshInstalled();
    void doInstall();
    void flashStatus(const QString& text, bool error);

    AddonRegistry* m_registry = nullptr;
    StremioAddonsProvider* m_provider = nullptr;
    BrowsePane* m_browse = nullptr;

    QPushButton* m_tabBtns[3] = {nullptr, nullptr, nullptr};
    QStackedWidget* m_panes = nullptr;
    QLineEdit* m_search = nullptr;   // marketplace search
    QLineEdit* m_urlInput = nullptr; // paste-to-install
    QPushButton* m_adultBtn = nullptr;
    QLabel* m_status = nullptr;
    QWidget* m_installedHost = nullptr;
    QVBoxLayout* m_installedLayout = nullptr;
    bool m_adult = false;
};

} // namespace tankoban
