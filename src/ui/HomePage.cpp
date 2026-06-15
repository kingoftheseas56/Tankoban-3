// Tankoban 3 — HomePage (Step 3). See HomePage.h.

#include "ui/HomePage.h"

#include "core/AddonClient.h"
#include "ui/CatalogRow.h"

#include <QFrame>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

namespace tankoban {

namespace {
const QString kCinemeta = QStringLiteral("https://v3-cinemeta.strem.io");
} // namespace

HomePage::HomePage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("ContentPage"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("HomeScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    outer->addWidget(scroll);

    auto* page = new QWidget(scroll);
    page->setObjectName(QStringLiteral("ContentPage"));
    page->setAttribute(Qt::WA_StyledBackground, true);

    auto* col = new QVBoxLayout(page);
    col->setContentsMargins(40, 32, 40, 32);
    col->setSpacing(34);

    m_popularMovies = new CatalogRow(QStringLiteral("Popular Movies"), page);
    m_popularSeries = new CatalogRow(QStringLiteral("Popular Series"), page);
    col->addWidget(m_popularMovies);
    col->addWidget(m_popularSeries);
    col->addStretch();

    scroll->setWidget(page);

    m_addons = new AddonClient(this);
    connect(m_addons, &AddonClient::catalogReady, this,
            [this](const QString& key, const QVector<MetaItem>& items) {
                if (key == QLatin1String("movies"))
                    m_popularMovies->setItems(items);
                else if (key == QLatin1String("series"))
                    m_popularSeries->setItems(items);
            });
    connect(m_addons, &AddonClient::catalogFailed, this,
            [this](const QString& key, const QString& err) {
                if (key == QLatin1String("movies"))
                    m_popularMovies->setStatus(QStringLiteral("Failed: ") + err);
                else if (key == QLatin1String("series"))
                    m_popularSeries->setStatus(QStringLiteral("Failed: ") + err);
            });

    m_addons->fetchCatalog(QStringLiteral("movies"), kCinemeta,
                           QStringLiteral("movie"), QStringLiteral("top"));
    m_addons->fetchCatalog(QStringLiteral("series"), kCinemeta,
                           QStringLiteral("series"), QStringLiteral("top"));
}

} // namespace tankoban
