#include "chrome/SubtitleMenu.h"

#include "util/Theme.h"

#include <QAbstractButton>
#include <QApplication>
#include <QEnterEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHash>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

// ── Harbor token palette (matches AudioMenu.cpp so the two popups read alike) ──
QColor popupBg()     { return QColor(17, 20, 25, 247); }   // bg-elevated/97
QColor popupBorder() { return QColor(255, 255, 255, 22); } // border-edge
QColor edgeSoft()    { return QColor(255, 255, 255, 16); } // border-edge-soft
QColor railTint()    { return QColor(0, 0, 0, 70); }       // bg-canvas/30 over elevated
QColor filterTint()  { return QColor(0, 0, 0, 36); }       // bg-canvas/15
QColor raisedBg()    { return QColor(255, 255, 255, 18); }
QColor elevatedBg()  { return QColor(255, 255, 255, 26); }
QColor textInk()     { return QColor(245, 247, 250); }
QColor textSubtle()  { return QColor(164, 172, 183); }
QColor textMuted()   { return QColor(131, 139, 151); }
QColor emerald()     { return QColor(52, 211, 153); }      // bg-emerald-400

const QString kAllLangs = QStringLiteral("__all__");

QPointF lucidePoint(const QRectF& rect, qreal x, qreal y)
{
    const qreal scale = std::min(rect.width(), rect.height()) / 24.0;
    const qreal ox = rect.x() + (rect.width() - 24.0 * scale) * 0.5;
    const qreal oy = rect.y() + (rect.height() - 24.0 * scale) * 0.5;
    return QPointF(ox + x * scale, oy + y * scale);
}

void drawSegment(QPainter& p, const QRectF& rect, qreal x1, qreal y1, qreal x2, qreal y2)
{
    p.drawLine(lucidePoint(rect, x1, y1), lucidePoint(rect, x2, y2));
}

double roundedDelay(double sec) { return std::round(sec * 100.0) / 100.0; }

QString langKeyOf(const TrackInfo& t)
{
    const QString k = t.lang.trimmed().toLower();
    return k.isEmpty() ? QStringLiteral("unknown") : k;
}

// Small display-name map; Harbor uses a full i18n DB (languageName), but phase 1
// only needs honest, readable labels (source map: keep this lean).
QString languageName(const QString& lang)
{
    const QString k = lang.trimmed().toLower();
    if (k.isEmpty()) return QStringLiteral("Unknown");
    static const QHash<QString, QString> kMap = {
        {QStringLiteral("en"), QStringLiteral("English")},  {QStringLiteral("eng"), QStringLiteral("English")},
        {QStringLiteral("es"), QStringLiteral("Spanish")},  {QStringLiteral("spa"), QStringLiteral("Spanish")},
        {QStringLiteral("fr"), QStringLiteral("French")},   {QStringLiteral("fre"), QStringLiteral("French")},
        {QStringLiteral("fra"), QStringLiteral("French")},
        {QStringLiteral("de"), QStringLiteral("German")},   {QStringLiteral("ger"), QStringLiteral("German")},
        {QStringLiteral("deu"), QStringLiteral("German")},
        {QStringLiteral("it"), QStringLiteral("Italian")},  {QStringLiteral("ita"), QStringLiteral("Italian")},
        {QStringLiteral("pt"), QStringLiteral("Portuguese")},{QStringLiteral("por"), QStringLiteral("Portuguese")},
        {QStringLiteral("ru"), QStringLiteral("Russian")},  {QStringLiteral("rus"), QStringLiteral("Russian")},
        {QStringLiteral("ja"), QStringLiteral("Japanese")}, {QStringLiteral("jpn"), QStringLiteral("Japanese")},
        {QStringLiteral("ko"), QStringLiteral("Korean")},   {QStringLiteral("kor"), QStringLiteral("Korean")},
        {QStringLiteral("zh"), QStringLiteral("Chinese")},  {QStringLiteral("zho"), QStringLiteral("Chinese")},
        {QStringLiteral("chi"), QStringLiteral("Chinese")},
        {QStringLiteral("ar"), QStringLiteral("Arabic")},   {QStringLiteral("ara"), QStringLiteral("Arabic")},
        {QStringLiteral("hi"), QStringLiteral("Hindi")},    {QStringLiteral("hin"), QStringLiteral("Hindi")},
        {QStringLiteral("nl"), QStringLiteral("Dutch")},    {QStringLiteral("dut"), QStringLiteral("Dutch")},
        {QStringLiteral("pl"), QStringLiteral("Polish")},   {QStringLiteral("pol"), QStringLiteral("Polish")},
        {QStringLiteral("tr"), QStringLiteral("Turkish")},  {QStringLiteral("tur"), QStringLiteral("Turkish")},
        {QStringLiteral("sv"), QStringLiteral("Swedish")},  {QStringLiteral("und"), QStringLiteral("Unknown")},
    };
    auto it = kMap.constFind(k);
    if (it != kMap.constEnd()) return it.value();
    if (k.size() <= 3) return k.toUpper();
    QString cap = lang.trimmed();
    cap[0] = cap[0].toUpper();
    return cap;
}

QString langBadge(const QString& lang)
{
    if (lang.trimmed().isEmpty()) return QStringLiteral("UND");
    return lang.simplified().remove(' ').left(3).toUpper();
}

QString variantTitle(const TrackInfo& t)
{
    if (!t.title.trimmed().isEmpty()) return t.title.trimmed();
    return t.external ? QStringLiteral("External subtitle") : QStringLiteral("Embedded track");
}

QString sourceLabel(const TrackInfo& t)
{
    return t.external ? QStringLiteral("External") : QStringLiteral("Embedded");
}

// Harbor pickReleaseHint: surface a filename-ish title as a mono hint line.
QString releaseHint(const TrackInfo& t)
{
    const QString s = t.title.trimmed();
    if (s.isEmpty()) return QString();
    static const QRegularExpression ext(QStringLiteral(R"(\.(srt|vtt|ass|ssa|sub)$)"),
                                        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression rel(QStringLiteral(R"([-.][A-Z0-9]{2,})"));
    if (ext.match(s).hasMatch()) return s;
    if (rel.match(s).hasMatch()) return s;
    if (s.size() > 24) return s;
    return QString();
}

struct LangGroup {
    QString key;
    QString display;
    QVector<TrackInfo> variants;
};

QVector<LangGroup> groupByLang(const QVector<TrackInfo>& tracks)
{
    QVector<LangGroup> groups;
    for (const TrackInfo& t : tracks) {
        const QString key = langKeyOf(t);
        int idx = -1;
        for (int i = 0; i < groups.size(); ++i)
            if (groups[i].key == key) { idx = i; break; }
        if (idx < 0) {
            groups.push_back({key, languageName(t.lang), {}});
            idx = groups.size() - 1;
        }
        groups[idx].variants.push_back(t);
    }
    return groups;
}

// ── Close / Reset icon button (same as AudioMenu) ─────────────────────────────
class PopupIconButton final : public QAbstractButton {
public:
    enum class Icon { Close, Reset };
    PopupIconButton(Icon icon, QWidget* parent = nullptr) : QAbstractButton(parent), m_icon(icon)
    {
        setFixedSize(m_icon == Icon::Close ? 28 : 24, m_icon == Icon::Close ? 28 : 24);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        if (underMouse() || isDown()) {
            p.setPen(Qt::NoPen);
            p.setBrush(raisedBg());
            p.drawEllipse(rect().adjusted(0, 0, -1, -1));
        }
        p.setPen(QPen(underMouse() ? textInk() : textSubtle(), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (m_icon == Icon::Close) {
            const QRectF r(7, 7, 14, 14);
            drawSegment(p, r, 5, 5, 19, 19);
            drawSegment(p, r, 19, 5, 5, 19);
            return;
        }
        const QRectF r(4, 4, 16, 16);
        p.drawArc(r.adjusted(1, 1, -1, -1), 30 * 16, 280 * 16);
        QPainterPath arrow;
        arrow.moveTo(lucidePoint(r, 17, 4));
        arrow.lineTo(lucidePoint(r, 20, 5));
        arrow.lineTo(lucidePoint(r, 18, 8));
        p.drawPath(arrow);
    }

private:
    Icon m_icon;
};

// ── Left-rail Off / On row ────────────────────────────────────────────────────
class OffOnButton final : public QAbstractButton {
public:
    explicit OffOnButton(QWidget* parent = nullptr) : QAbstractButton(parent)
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
        setFixedHeight(34);
    }
    void setOff(bool off) { if (m_off != off) { m_off = off; setEnabled(!off); update(); } }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRect r = rect().adjusted(0, 0, -1, -1);
        if (!m_off) {
            p.setPen(QPen(popupBorder(), 1.0));
            p.setBrush(elevatedBg());
            p.drawRoundedRect(r, 6, 6);
        } else if (underMouse()) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 8));
            p.drawRoundedRect(r, 6, 6);
        }
        const QRect circle(10, height() / 2 - 8, 16, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(m_off ? raisedBg() : theme::accent());
        p.drawEllipse(circle);
        if (!m_off) {
            p.setPen(QPen(QColor(19, 21, 27), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawLine(circle.left() + 4, circle.center().y(), circle.left() + 7, circle.bottom() - 4);
            p.drawLine(circle.left() + 7, circle.bottom() - 4, circle.right() - 3, circle.top() + 4);
        }
        QFont f(QStringLiteral("Inter"));
        f.setPointSizeF(9.6);
        f.setWeight(QFont::DemiBold);
        p.setFont(f);
        p.setPen(m_off ? textSubtle() : textInk());
        p.drawText(QRect(circle.right() + 10, 0, width() - circle.right() - 16, height()),
                   Qt::AlignLeft | Qt::AlignVCenter, m_off ? QStringLiteral("Off") : QStringLiteral("On"));
    }

private:
    bool m_off = true;
};

// ── Left-rail language group button (also handles the "All languages" row) ────
class LangButton final : public QAbstractButton {
public:
    LangButton(QString key, QString display, QString badge, int count, bool allLangs,
               QWidget* parent = nullptr)
        : QAbstractButton(parent), m_key(std::move(key)), m_display(std::move(display)),
          m_badge(std::move(badge)), m_count(count), m_all(allLangs)
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
        setFixedHeight(34);
    }
    QString langKey() const { return m_key; }
    void setActive(bool a) { if (m_active != a) { m_active = a; update(); } }
    void setHasSelected(bool s) { if (m_hasSelected != s) { m_hasSelected = s; update(); } }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRect r = rect().adjusted(0, 0, -1, -1);
        if (m_active) {
            p.setPen(QPen(popupBorder(), 1.0));
            p.setBrush(elevatedBg());
            p.drawRoundedRect(r, 6, 6);
        } else if (underMouse()) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 12));
            p.drawRoundedRect(r, 6, 6);
        }

        int x = 10;
        if (m_all) {
            // Languages glyph (lucide) — two short bars suggesting text lines.
            const QRectF g(x, height() / 2.0 - 8, 16, 16);
            p.setPen(QPen(m_active ? textInk() : textMuted(), 1.7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            drawSegment(p, g, 4, 6, 12, 6);
            drawSegment(p, g, 4, 11, 14, 11);
            drawSegment(p, g, 4, 16, 10, 16);
            x += 16 + 8;
        } else {
            QFont bf(QStringLiteral("Inter"));
            bf.setPointSizeF(7.6);
            bf.setBold(true);
            p.setFont(bf);
            QFontMetrics bfm(bf);
            const int bw = bfm.horizontalAdvance(m_badge) + 10;
            const QRect badgeRect(x, height() / 2 - 9, bw, 18);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 20));
            p.drawRoundedRect(badgeRect, 5, 5);
            p.setPen(textSubtle());
            p.drawText(badgeRect, Qt::AlignCenter, m_badge);
            x = badgeRect.right() + 8;
        }

        int rightReserve = 16; // count
        if (m_hasSelected) rightReserve += 12;
        QFont nf(QStringLiteral("Inter"));
        nf.setPointSizeF(9.6);
        nf.setWeight(QFont::Medium);
        p.setFont(nf);
        QFontMetrics nfm(nf);
        const int nameW = width() - x - rightReserve - 8;
        const QString name = nfm.elidedText(m_display, Qt::ElideRight, std::max(10, nameW));
        p.setPen(m_active ? textInk() : textMuted());
        p.drawText(QRect(x, 0, std::max(10, nameW), height()), Qt::AlignLeft | Qt::AlignVCenter, name);

        int rx = width() - 12;
        QFont cf(QStringLiteral("Inter"));
        cf.setPointSizeF(8.4);
        p.setFont(cf);
        p.setPen(textSubtle());
        const QString cnt = QString::number(m_count);
        const int cw = QFontMetrics(cf).horizontalAdvance(cnt);
        p.drawText(QRect(rx - cw, 0, cw, height()), Qt::AlignRight | Qt::AlignVCenter, cnt);
        rx -= cw + 6;
        if (m_hasSelected) {
            p.setPen(Qt::NoPen);
            p.setBrush(theme::accent());
            p.drawEllipse(QRect(rx - 6, height() / 2 - 3, 6, 6));
        }
    }

private:
    QString m_key, m_display, m_badge;
    int m_count = 0;
    bool m_all = false;
    bool m_active = false;
    bool m_hasSelected = false;
};

// ── Right-panel embedded variant row ──────────────────────────────────────────
class VariantRowButton final : public QAbstractButton {
public:
    explicit VariantRowButton(const TrackInfo& track, QWidget* parent = nullptr)
        : QAbstractButton(parent), m_track(track)
    {
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setMouseTracking(true);
        m_release = releaseHint(track);
        setFixedHeight(m_release.isEmpty() ? 48 : 64);
    }
    void setSelectedTrack(bool s) { if (m_selected != s) { m_selected = s; update(); } }
    QString trackId() const { return m_track.id; }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRect r = rect().adjusted(0, 0, -1, -1);
        if (m_selected) {
            p.setPen(QPen(popupBorder(), 1.0));
            p.setBrush(elevatedBg());
            p.drawRoundedRect(r, 8, 8);
        } else if (underMouse()) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 10));
            p.drawRoundedRect(r, 8, 8);
        }

        const QRect circle(10, 9, 16, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(m_selected ? theme::accent() : raisedBg());
        p.drawEllipse(circle);
        if (m_selected) {
            p.setPen(QPen(QColor(19, 21, 27), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawLine(circle.left() + 4, circle.center().y(), circle.left() + 7, circle.bottom() - 4);
            p.drawLine(circle.left() + 7, circle.bottom() - 4, circle.right() - 3, circle.top() + 4);
        }

        const int textLeft = circle.right() + 11;
        const int textW = width() - textLeft - 12;

        QFont titleFont(QStringLiteral("Inter"));
        titleFont.setPointSizeF(9.8);
        titleFont.setWeight(QFont::Medium);
        p.setFont(titleFont);
        p.setPen(textInk());
        const QString title = QFontMetrics(titleFont).elidedText(variantTitle(m_track), Qt::ElideRight, textW);
        p.drawText(QRect(textLeft, 7, textW, 16), Qt::AlignLeft | Qt::AlignVCenter, title);

        int metaY = 25;
        if (!m_release.isEmpty()) {
            QFont mono(QStringLiteral("Consolas"));
            mono.setPointSizeF(8.2);
            p.setFont(mono);
            p.setPen(textMuted());
            const QString rel = QFontMetrics(mono).elidedText(m_release, Qt::ElideRight, textW);
            p.drawText(QRect(textLeft, 24, textW, 14), Qt::AlignLeft | Qt::AlignVCenter, rel);
            metaY = 41;
        }

        // Meta line: LANG · Source · CODEC · [tags]
        int x = textLeft;
        const int metaH = 14;
        QFont metaFont(QStringLiteral("Inter"));
        metaFont.setPointSizeF(8.2);
        QFontMetrics mfm(metaFont);
        auto drawText = [&](const QString& s, const QColor& c, bool bold) {
            QFont f = metaFont;
            f.setBold(bold);
            p.setFont(f);
            p.setPen(c);
            const int w = QFontMetrics(f).horizontalAdvance(s);
            if (x + w > width() - 10) return false;
            p.drawText(QRect(x, metaY, w, metaH), Qt::AlignLeft | Qt::AlignVCenter, s);
            x += w + 6;
            return true;
        };
        auto drawDot = [&]() {
            p.setPen(Qt::NoPen);
            p.setBrush(textMuted());
            p.drawEllipse(QRectF(x + 1.0, metaY + metaH / 2.0 - 1.0, 2.0, 2.0));
            x += 2 + 8;
        };
        auto drawChip = [&](const QString& s, const QColor& bg, const QColor& fg) {
            QFont cf = metaFont;
            cf.setBold(true);
            const int w = QFontMetrics(cf).horizontalAdvance(s) + 10;
            if (x + w > width() - 10) return;
            const QRect chip(x, metaY, w, metaH);
            p.setPen(Qt::NoPen);
            p.setBrush(bg);
            p.drawRoundedRect(chip, 4, 4);
            p.setFont(cf);
            p.setPen(fg);
            p.drawText(chip, Qt::AlignCenter, s);
            x += w + 5;
        };

        drawText(languageName(m_track.lang).toUpper(), textSubtle(), true);
        drawDot();
        drawText(sourceLabel(m_track), textSubtle(), false);
        if (!m_track.codec.trimmed().isEmpty()) {
            drawDot();
            drawText(m_track.codec.toUpper(), textSubtle(), false);
        }
        if (m_track.forced)
            drawChip(QStringLiteral("FORCED"), QColor(56, 189, 248, 38), QColor(186, 230, 253));
        if (m_track.hearingImpaired)
            drawChip(QStringLiteral("HI/SDH"), QColor(251, 191, 36, 38), QColor(253, 230, 138));
        if (m_track.isDefault)
            drawChip(QStringLiteral("DEFAULT"), raisedBg(), textMuted());
    }

private:
    TrackInfo m_track;
    QString m_release;
    bool m_selected = false;
};

} // namespace

// ── The floating popup ────────────────────────────────────────────────────────
class SubtitlePopup final : public QWidget {
public:
    static constexpr int kHeaderH = 44;
    static constexpr int kRailW = 128;
    static constexpr int kFilterH = 38;
    static constexpr int kDelayH = 40;

    explicit SubtitlePopup(QWidget* parent = nullptr) : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(500, 400);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // Header --------------------------------------------------------------
        auto* header = new QWidget(this);
        header->setFixedHeight(kHeaderH);
        auto* hl = new QHBoxLayout(header);
        hl->setContentsMargins(16, 0, 10, 0);
        hl->setSpacing(8);
        auto* title = new QLabel(QStringLiteral("Subtitles"), header);
        QFont tf(QStringLiteral("Inter"));
        tf.setPointSizeF(10.6);
        tf.setWeight(QFont::DemiBold);
        title->setFont(tf);
        title->setStyleSheet(QStringLiteral("color: rgb(245,247,250); background: transparent;"));
        m_count = new QLabel(header);
        QFont cf(QStringLiteral("Inter"));
        cf.setPointSizeF(8.8);
        m_count->setFont(cf);
        m_count->setStyleSheet(QStringLiteral("color: rgb(164,172,183); background: transparent;"));
        auto* close = new PopupIconButton(PopupIconButton::Icon::Close, header);
        connect(close, &QAbstractButton::clicked, this, [this] { if (m_closeHandler) m_closeHandler(); });
        hl->addWidget(title);
        hl->addWidget(m_count);
        hl->addStretch(1);
        hl->addWidget(close);
        root->addWidget(header);

        // Body: left rail + right section ------------------------------------
        auto* body = new QWidget(this);
        auto* bl = new QHBoxLayout(body);
        bl->setContentsMargins(0, 0, 0, 0);
        bl->setSpacing(0);

        m_railScroll = new QScrollArea(body);
        m_railScroll->setFixedWidth(kRailW);
        m_railScroll->setFrameShape(QFrame::NoFrame);
        m_railScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_railScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_railScroll->setWidgetResizable(true);
        m_railScroll->setStyleSheet(scrollStyle());
        m_railHost = new QWidget(m_railScroll);
        m_railLayout = new QVBoxLayout(m_railHost);
        m_railLayout->setContentsMargins(8, 8, 8, 8);
        m_railLayout->setSpacing(2);
        m_railScroll->setWidget(m_railHost);
        bl->addWidget(m_railScroll);

        auto* section = new QWidget(body);
        auto* sl = new QVBoxLayout(section);
        sl->setContentsMargins(0, 0, 0, 0);
        sl->setSpacing(0);

        m_filterRow = new QWidget(section);
        m_filterRow->setFixedHeight(kFilterH);
        auto* fl = new QHBoxLayout(m_filterRow);
        fl->setContentsMargins(12, 0, 12, 0);
        fl->setSpacing(6);
        m_tabAll = makeTab(QStringLiteral("All"));
        m_tabEmbedded = makeTab(QStringLiteral("Embedded"));
        m_tabExternal = makeTab(QStringLiteral("External"));
        fl->addWidget(m_tabAll);
        fl->addWidget(m_tabEmbedded);
        fl->addWidget(m_tabExternal);
        fl->addStretch(1);
        connect(m_tabAll, &QPushButton::clicked, this, [this] { setFilter(QStringLiteral("all")); });
        connect(m_tabEmbedded, &QPushButton::clicked, this, [this] { setFilter(QStringLiteral("embedded")); });
        sl->addWidget(m_filterRow);

        m_variantScroll = new QScrollArea(section);
        m_variantScroll->setFrameShape(QFrame::NoFrame);
        m_variantScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_variantScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_variantScroll->setWidgetResizable(true);
        m_variantScroll->setStyleSheet(scrollStyle());
        m_variantHost = new QWidget(m_variantScroll);
        m_variantLayout = new QVBoxLayout(m_variantHost);
        m_variantLayout->setContentsMargins(8, 8, 8, 8);
        m_variantLayout->setSpacing(2);
        m_variantScroll->setWidget(m_variantHost);
        sl->addWidget(m_variantScroll, 1);

        auto* delayRow = new QWidget(section);
        delayRow->setFixedHeight(kDelayH);
        auto* dl = new QHBoxLayout(delayRow);
        dl->setContentsMargins(12, 0, 10, 0);
        dl->setSpacing(4);
        auto* sync = new QLabel(QStringLiteral("SYNC"), delayRow);
        QFont sf(QStringLiteral("Inter"));
        sf.setPointSizeF(7.8);
        sf.setBold(true);
        sf.setCapitalization(QFont::AllUppercase);
        sync->setFont(sf);
        sync->setStyleSheet(QStringLiteral("color: rgb(131,139,151); background: transparent;"));
        dl->addWidget(sync);
        const double steps[] = {-0.5, -0.1, -0.05};
        for (double s : steps) dl->addWidget(makeStep(delayRow, s));
        m_delayValue = new QLabel(delayRow);
        QFont mono(QStringLiteral("Consolas"));
        mono.setPointSizeF(9.3);
        m_delayValue->setFont(mono);
        m_delayValue->setAlignment(Qt::AlignCenter);
        m_delayValue->setStyleSheet(QStringLiteral("color: rgb(245,247,250); background: transparent;"));
        dl->addWidget(m_delayValue, 1);
        const double steps2[] = {0.05, 0.1, 0.5};
        for (double s : steps2) dl->addWidget(makeStep(delayRow, s));
        m_reset = new PopupIconButton(PopupIconButton::Icon::Reset, delayRow);
        connect(m_reset, &QAbstractButton::clicked, this, [this] { if (m_delayHandler) m_delayHandler(0.0); });
        dl->addWidget(m_reset);
        sl->addWidget(delayRow);

        bl->addWidget(section, 1);
        root->addWidget(body, 1);
    }

    void setSnapshot(const PlayerSnapshot& snap)
    {
        m_snapshot = snap;
        m_groups = groupByLang(snap.subtitleTracks);
        // Resolve the active language: keep if valid, else jump to the group that
        // holds the selected track, else the first group (Harbor's effect order).
        const QString sel = selectedId();
        if (m_activeLang != kAllLangs) {
            bool valid = false;
            for (const auto& g : m_groups) if (g.key == m_activeLang) { valid = true; break; }
            if (!valid) {
                m_activeLang.clear();
                for (const auto& g : m_groups)
                    for (const auto& v : g.variants)
                        if (v.id == sel) { m_activeLang = g.key; break; }
                if (m_activeLang.isEmpty() && !m_groups.isEmpty()) m_activeLang = m_groups.front().key;
            }
        }
        rebuildRail();
        rebuildVariants();
        refreshDelay();
        refreshTabs();
    }

    void setTrackHandler(std::function<void(const QString&)> fn) { m_trackHandler = std::move(fn); }
    void setDelayHandler(std::function<void(double)> fn) { m_delayHandler = std::move(fn); }
    void setCloseHandler(std::function<void()> fn) { m_closeHandler = std::move(fn); }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QPainterPath shape;
        shape.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 16, 16);
        p.fillPath(shape, popupBg());
        p.setClipPath(shape);

        // Region tints (left rail, filter strip, delay strip).
        p.fillRect(QRect(0, kHeaderH, kRailW, height() - kHeaderH), railTint());
        const bool hasTracks = !m_snapshot.subtitleTracks.isEmpty();
        if (hasTracks)
            p.fillRect(QRect(kRailW, kHeaderH, width() - kRailW, kFilterH), filterTint());
        p.fillRect(QRect(kRailW, height() - kDelayH, width() - kRailW, kDelayH), railTint());

        // Separator lines.
        p.setPen(QPen(edgeSoft(), 1.0));
        p.drawLine(0, kHeaderH, width(), kHeaderH);
        p.drawLine(kRailW, kHeaderH, kRailW, height());
        if (hasTracks)
            p.drawLine(kRailW, kHeaderH + kFilterH, width(), kHeaderH + kFilterH);
        p.drawLine(kRailW, height() - kDelayH, width(), height() - kDelayH);

        p.setClipping(false);
        p.setPen(QPen(popupBorder(), 1.0));
        p.setBrush(Qt::NoBrush);
        p.drawPath(shape);
    }

private:
    static QString scrollStyle()
    {
        return QStringLiteral(
            "QScrollArea { background: transparent; }"
            "QScrollBar:vertical { width: 9px; background: transparent; margin: 4px 2px 4px 0; }"
            "QScrollBar::handle:vertical { background: rgba(255,255,255,0.18); border-radius: 4px; min-height: 28px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
    }

    QPushButton* makeTab(const QString& text)
    {
        auto* b = new QPushButton(text, this);
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);
        b->setCheckable(true);
        b->setFixedHeight(26);
        b->setStyleSheet(QStringLiteral(
            "QPushButton {"
            " background: transparent; color: rgb(164,172,183);"
            " border: 1px solid transparent; border-radius: 13px; padding: 0 12px;"
            " font-family: Inter; font-size: 11px; font-weight: 600; }"
            "QPushButton:hover:!checked { background: rgba(255,255,255,0.07); color: rgb(245,247,250); }"
            "QPushButton:checked {"
            " background: rgba(255,255,255,0.10); color: rgb(245,247,250);"
            " border: 1px solid rgba(255,255,255,0.22); }"
            "QPushButton:disabled { color: rgba(164,172,183,0.4); }"));
        return b;
    }

    QPushButton* makeStep(QWidget* parent, double delta)
    {
        const QString label = QStringLiteral("%1%2")
                                  .arg(delta < 0 ? QStringLiteral("-") : QStringLiteral("+"))
                                  .arg(QString::number(std::abs(delta), 'g', 4));
        auto* b = new QPushButton(label, parent);
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);
        b->setFixedHeight(24);
        b->setStyleSheet(QStringLiteral(
            "QPushButton {"
            " background: rgba(255,255,255,0.07); color: rgb(245,247,250);"
            " border: none; border-radius: 6px; padding: 0 7px;"
            " font-family: Inter; font-size: 10.5px; font-weight: 600; }"
            "QPushButton:hover { background: rgba(255,255,255,0.12); }"));
        connect(b, &QPushButton::clicked, this, [this, delta] {
            if (m_delayHandler) m_delayHandler(roundedDelay(m_snapshot.subDelaySec + delta));
        });
        return b;
    }

    QString selectedId() const
    {
        for (const TrackInfo& t : m_snapshot.subtitleTracks)
            if (t.selected) return t.id;
        return QString();
    }
    bool offSelected() const { return selectedId().isEmpty(); }

    void clearLayout(QVBoxLayout* layout)
    {
        while (QLayoutItem* item = layout->takeAt(0)) {
            if (QWidget* w = item->widget()) w->deleteLater();
            delete item;
        }
    }

    void setActiveLang(const QString& key)
    {
        if (m_activeLang == key) return;
        m_activeLang = key;
        rebuildRail();
        rebuildVariants();
    }

    void setFilter(const QString& f)
    {
        if (m_filter == f) return;
        m_filter = f;
        rebuildVariants();
        refreshTabs();
    }

    void rebuildRail()
    {
        clearLayout(m_railLayout);
        const QString sel = selectedId();

        auto* off = new OffOnButton(m_railHost);
        off->setOff(offSelected());
        connect(off, &QAbstractButton::clicked, this, [this] {
            if (offSelected()) return;
            if (m_trackHandler) m_trackHandler(QString());  // -> mpv sid=no
            if (m_closeHandler) m_closeHandler();
        });
        m_railLayout->addWidget(off);

        if (!m_groups.isEmpty()) {
            auto* label = new QLabel(QStringLiteral("LANGUAGES"), m_railHost);
            QFont lf(QStringLiteral("Inter"));
            lf.setPointSizeF(7.4);
            lf.setBold(true);
            label->setFont(lf);
            label->setContentsMargins(8, 8, 8, 2);
            label->setStyleSheet(QStringLiteral("color: rgb(131,139,151); background: transparent;"
                                                " letter-spacing: 1px;"));
            m_railLayout->addWidget(label);
        }

        if (m_groups.size() > 1) {
            auto* all = new LangButton(kAllLangs, QStringLiteral("All languages"), QString(),
                                       m_snapshot.subtitleTracks.size(), true, m_railHost);
            all->setActive(m_activeLang == kAllLangs);
            connect(all, &QAbstractButton::clicked, this, [this] { setActiveLang(kAllLangs); });
            m_railLayout->addWidget(all);
        }

        for (const LangGroup& g : m_groups) {
            bool hasSel = false;
            for (const auto& v : g.variants) if (v.id == sel) { hasSel = true; break; }
            QString badge = g.variants.isEmpty() ? QStringLiteral("UND") : langBadge(g.variants.front().lang);
            auto* b = new LangButton(g.key, g.display, badge, g.variants.size(), false, m_railHost);
            b->setActive(m_activeLang == g.key);
            b->setHasSelected(hasSel);
            const QString key = g.key;
            connect(b, &QAbstractButton::clicked, this, [this, key] { setActiveLang(key); });
            m_railLayout->addWidget(b);
        }
        m_railLayout->addStretch(1);
    }

    QVector<TrackInfo> visibleVariants() const
    {
        QVector<TrackInfo> list;
        if (m_activeLang == kAllLangs) {
            list = m_snapshot.subtitleTracks;
        } else {
            for (const LangGroup& g : m_groups)
                if (g.key == m_activeLang) { list = g.variants; break; }
        }
        QVector<TrackInfo> out;
        for (const TrackInfo& t : list) {
            if (m_filter == QStringLiteral("embedded") && t.external) continue;
            if (m_filter == QStringLiteral("external") && !t.external) continue;
            out.push_back(t);
        }
        return out;
    }

    void rebuildVariants()
    {
        clearLayout(m_variantLayout);
        const QString sel = selectedId();

        if (m_snapshot.subtitleTracks.isEmpty()) {
            auto* empty = new QLabel(QStringLiteral("No embedded subtitles in this file."), m_variantHost);
            empty->setWordWrap(true);
            empty->setContentsMargins(12, 14, 12, 12);
            empty->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            QFont ef(QStringLiteral("Inter"));
            ef.setPointSizeF(9.6);
            empty->setFont(ef);
            empty->setStyleSheet(QStringLiteral("color: rgb(131,139,151); background: transparent;"));
            m_variantLayout->addWidget(empty);
            m_variantLayout->addStretch(1);
            m_count->setVisible(false);
            return;
        }

        const QVector<TrackInfo> vis = visibleVariants();
        if (vis.isEmpty()) {
            auto* none = new QLabel(QStringLiteral("No tracks match this filter."), m_variantHost);
            none->setContentsMargins(12, 14, 12, 12);
            QFont nf(QStringLiteral("Inter"));
            nf.setPointSizeF(9.6);
            none->setFont(nf);
            none->setStyleSheet(QStringLiteral("color: rgb(131,139,151); background: transparent;"));
            m_variantLayout->addWidget(none);
        } else {
            for (const TrackInfo& t : vis) {
                auto* row = new VariantRowButton(t, m_variantHost);
                row->setSelectedTrack(t.id == sel && !sel.isEmpty());
                connect(row, &QAbstractButton::clicked, this, [this, row] {
                    if (m_trackHandler) m_trackHandler(row->trackId());
                    if (m_closeHandler) m_closeHandler();
                });
                m_variantLayout->addWidget(row);
            }
        }
        m_variantLayout->addStretch(1);
        m_count->setText(QString::number(m_snapshot.subtitleTracks.size()));
        m_count->setVisible(true);
    }

    void refreshTabs()
    {
        const bool hasTracks = !m_snapshot.subtitleTracks.isEmpty();
        m_filterRow->setVisible(hasTracks);
        int embedded = 0, external = 0;
        for (const TrackInfo& t : m_snapshot.subtitleTracks) (t.external ? external : embedded)++;
        m_tabExternal->setEnabled(external > 0);   // phase 1: no externals -> disabled
        m_tabEmbedded->setEnabled(embedded > 0);
        m_tabAll->setChecked(m_filter == QStringLiteral("all"));
        m_tabEmbedded->setChecked(m_filter == QStringLiteral("embedded"));
        m_tabExternal->setChecked(m_filter == QStringLiteral("external"));
    }

    void refreshDelay()
    {
        m_delayValue->setText(QStringLiteral("%1%2s")
                                  .arg(m_snapshot.subDelaySec >= 0.0 ? QStringLiteral("+") : QString())
                                  .arg(m_snapshot.subDelaySec, 0, 'f', 2));
        m_reset->setVisible(std::abs(m_snapshot.subDelaySec) > 0.0001);
    }

    PlayerSnapshot m_snapshot;
    QVector<LangGroup> m_groups;
    QString m_activeLang;
    QString m_filter = QStringLiteral("all");

    QLabel* m_count = nullptr;
    QScrollArea* m_railScroll = nullptr;
    QWidget* m_railHost = nullptr;
    QVBoxLayout* m_railLayout = nullptr;
    QWidget* m_filterRow = nullptr;
    QPushButton* m_tabAll = nullptr;
    QPushButton* m_tabEmbedded = nullptr;
    QPushButton* m_tabExternal = nullptr;
    QScrollArea* m_variantScroll = nullptr;
    QWidget* m_variantHost = nullptr;
    QVBoxLayout* m_variantLayout = nullptr;
    QLabel* m_delayValue = nullptr;
    PopupIconButton* m_reset = nullptr;

    std::function<void(const QString&)> m_trackHandler;
    std::function<void(double)> m_delayHandler;
    std::function<void()> m_closeHandler;
};

// ── The 48x48 circular button ─────────────────────────────────────────────────
SubtitleMenu::SubtitleMenu(QWidget* parent) : QWidget(parent)
{
    setFixedSize(48, 48);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);

    m_popup = new SubtitlePopup();
    m_popup->setTrackHandler([this](const QString& id) { emit trackRequested(id); hidePopup(); });
    m_popup->setDelayHandler([this](double sec) {
        m_snapshot.subDelaySec = sec;
        m_popup->setSnapshot(m_snapshot);
        emit delayRequested(sec);
    });
    m_popup->setCloseHandler([this] { hidePopup(); });

    qApp->installEventFilter(this);
}

SubtitleMenu::~SubtitleMenu()
{
    qApp->removeEventFilter(this);
    delete m_popup;
}

bool SubtitleMenu::hasSelectedSub() const
{
    for (const TrackInfo& t : m_snapshot.subtitleTracks)
        if (t.selected) return true;
    return false;
}

void SubtitleMenu::setSnapshot(const PlayerSnapshot& snap)
{
    m_snapshot = snap;
    if (m_popup) m_popup->setSnapshot(snap);
    update();
}

void SubtitleMenu::syncPopupPosition()
{
    if (!m_open || !m_popup) return;

    QScreen* screen = window() ? window()->screen() : QGuiApplication::primaryScreen();
    const QRect available = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 720);

    QPoint pos = mapToGlobal(QPoint(width() - m_popup->width(), -10 - m_popup->height()));
    if (pos.x() < available.left() + 16) pos.setX(available.left() + 16);
    if (pos.x() + m_popup->width() > available.right() - 15)
        pos.setX(available.right() - 15 - m_popup->width());
    if (pos.y() < available.top() + 16)
        pos = mapToGlobal(QPoint(width() - m_popup->width(), height() + 10));
    if (pos.y() + m_popup->height() > available.bottom() - 15)
        pos.setY(std::max(available.top() + 16, available.bottom() - 15 - m_popup->height()));

    m_popup->move(pos);
}

bool SubtitleMenu::eventFilter(QObject*, QEvent* event)
{
    if (!m_open || !m_popup) return false;

    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        const QPoint global = mouse->globalPosition().toPoint();
        if (!globalRect().contains(global) && !m_popup->frameGeometry().contains(global)) hidePopup();
    } else if (event->type() == QEvent::ApplicationDeactivate) {
        hidePopup();
    } else if (event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Escape) hidePopup();
    }
    return false;
}

void SubtitleMenu::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (m_open || m_hover || m_pressed) {
        p.setPen(Qt::NoPen);
        p.setBrush(m_open ? QColor(255, 255, 255, 56) : QColor(255, 255, 255, 26));
        p.drawEllipse(rect().adjusted(0, 0, -1, -1));
    }

    // lucide Captions glyph — a rounded frame with short caption lines.
    const QRectF iconRect(14, 14, 20, 20);
    p.setPen(QPen(QColor(255, 255, 255, m_open ? 255 : 217), 1.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QPainterPath frame;
    frame.addRoundedRect(QRectF(lucidePoint(iconRect, 3, 5), lucidePoint(iconRect, 21, 19)), 3, 3);
    p.drawPath(frame);
    drawSegment(p, iconRect, 7, 15, 11, 15);
    drawSegment(p, iconRect, 15, 15, 17, 15);
    drawSegment(p, iconRect, 7, 11, 9, 11);
    drawSegment(p, iconRect, 13, 11, 17, 11);

    if (hasSelectedSub()) {
        p.setPen(Qt::NoPen);
        p.setBrush(emerald());
        p.drawEllipse(QRectF(32, 10, 6, 6));   // end-2.5 top-2.5, h-1.5 w-1.5
    }
}

void SubtitleMenu::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    m_pressed = true;
    update();
}

void SubtitleMenu::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    const bool inside = rect().contains(event->position().toPoint());
    const bool wasPressed = m_pressed;
    m_pressed = false;
    update();
    if (wasPressed && inside) toggleOpen();
}

void SubtitleMenu::enterEvent(QEnterEvent* event)
{
    m_hover = true;
    update();
    QWidget::enterEvent(event);
}

void SubtitleMenu::leaveEvent(QEvent* event)
{
    m_hover = false;
    m_pressed = false;
    update();
    QWidget::leaveEvent(event);
}

void SubtitleMenu::setOpen(bool open)
{
    if (m_open == open) return;
    m_open = open;
    emit openChanged(open);
    update();
}

void SubtitleMenu::toggleOpen()
{
    if (m_open) hidePopup();
    else showPopup();
}

void SubtitleMenu::showPopup()
{
    if (!m_popup) return;
    m_popup->setSnapshot(m_snapshot);
    syncPopupPosition();
    m_popup->show();
    m_popup->raise();
    setOpen(true);
}

void SubtitleMenu::hidePopup()
{
    if (!m_popup) return;
    m_popup->hide();
    setOpen(false);
}

QRect SubtitleMenu::globalRect() const
{
    return QRect(mapToGlobal(QPoint(0, 0)), size());
}
