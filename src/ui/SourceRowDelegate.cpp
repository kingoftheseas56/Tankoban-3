// Tankoban 3 - see SourceRowDelegate.h. Variable-height, multi-line faithful port of
// StremioRow's paint: word-wrapped headline + description (Stremio name/title fields are
// multi-line), badge chips, status; content vertically centred; the row grows to fit its
// content (>= kMinRowHeight). Icons cached in QPixmapCache; geometry mirrors StremioRow's
// QHBoxLayout (margins 20, col gap 20, logo col 68 w/ 56 tile, actions copy 36 + play 64).
#include "ui/SourceRowDelegate.h"
#include "ui/SourceListModel.h"
#include "ui/StreamRowPaint.h"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPixmapCache>
#include <QSet>
#include <QSvgRenderer>

namespace tankoban {
namespace {
const QColor kRowBg(26, 29, 36);
const QColor kTile(26, 29, 36);
const QColor kHead(0xf3, 0xf1, 0xea);
const QColor kMuted(0xaa, 0xb1, 0xbd);
const QColor kGold(0xe8, 0xb9, 0x23);
const QColor kInk(0x12, 0x13, 0x17);

constexpr int kPad = 20;       // row content margins (StremioRow QHBoxLayout margins)
constexpr int kColGap = 20;    // gap between logo / content / actions
constexpr int kLogoCol = 68;   // logo column width (56 tile centred in it)
constexpr int kTileSz = 56;
constexpr int kContentGap = 6; // vertical gap between headline/desc/badges/status
constexpr int kCopySz = 36;
constexpr int kPlaySz = 64;
constexpr int kActionGap = 8;
constexpr int kActionsW = kCopySz + kActionGap + kPlaySz; // 108
constexpr int kBadgeH = 18;
constexpr int kBadgeTop = 2;

QFont headFont()   { QFont f(QStringLiteral("Inter")); f.setPixelSize(16); f.setWeight(QFont::DemiBold); return f; }
QFont descFont()   { QFont f(QStringLiteral("Inter")); f.setPixelSize(14); return f; }
QFont badgeFont()  { QFont f(QStringLiteral("Inter")); f.setPixelSize(11); f.setWeight(QFont::DemiBold); return f; }
QFont statusFont() { QFont f(QStringLiteral("Inter")); f.setPixelSize(13); f.setWeight(QFont::DemiBold); return f; }
QFont tileFont()   { QFont f(QStringLiteral("Inter")); f.setPixelSize(22); f.setWeight(QFont::Bold); return f; }

// Render an SVG glyph once into a cached pixmap (QPixmapCache, dpr-aware).
QPixmap glyph(const QString& key, const QString& inner, const QColor& color, int px, bool filled, qreal dpr)
{
    const QString ck = QStringLiteral("srd:%1:%2:%3").arg(key, color.name()).arg(int(px * dpr));
    QPixmap pm;
    if (QPixmapCache::find(ck, &pm)) return pm;
    const QString svg = filled
        ? QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='%1' stroke='none'>%2</svg>").arg(color.name(), inner)
        : QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%1' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>%2</svg>").arg(color.name(), inner);
    QSvgRenderer r(svg.toUtf8());
    pm = QPixmap(int(px * dpr), int(px * dpr));
    pm.fill(Qt::transparent);
    { QPainter pp(&pm); r.render(&pp); }
    pm.setDevicePixelRatio(dpr);
    QPixmapCache::insert(ck, pm);
    return pm;
}
const QString kPlayGlyph = QStringLiteral("<polygon points='8 5 19 12 8 19'/>");
const QString kCopyGlyph = QStringLiteral("<rect width='14' height='14' x='8' y='8' rx='2' ry='2'/><path d='M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2'/>");
const QString kCheckGlyph = QStringLiteral("<path d='M20 6 9 17l-5-5'/>");

// Text precedence — identical to StremioRow::setStream.
QString addonOf(const ScoredStream& s)    { return !s.addonName.isEmpty() ? s.addonName : s.addonId; }
QString headlineOf(const ScoredStream& s) { const QString h = s.name.trimmed(); return !h.isEmpty() ? h : addonOf(s); }
QString descOf(const ScoredStream& s)     { const QString t = s.title.trimmed(); return !t.isEmpty() ? t : s.description.trimmed(); }
QString statusOf(const ScoredStream& s, bool playable)
{
    const bool isTorrent = !s.infoHash.isEmpty();
    const bool hasDirect = !s.url.isEmpty() && s.url != QStringLiteral("#");
    if (isTorrent && !hasDirect)               return QStringLiteral("Torrent · P2P stream");
    if (!playable && !s.externalUrl.isEmpty()) return QStringLiteral("External source");
    return QString();
}

int contentWidthFor(int rowWidth) { return qMax(40, rowWidth - 2 * kPad - kLogoCol - 2 * kColGap - kActionsW); }

// Word-wrapped height of multi-line text at a given width (0 for empty).
int wrappedHeight(const QFont& font, const QString& text, int width)
{
    if (text.isEmpty()) return 0;
    QFontMetrics fm(font);
    return fm.boundingRect(QRect(0, 0, width, 100000), Qt::TextWordWrap, text).height();
}

// Stacked height of the content column (headline + desc + badges + status) at width cw.
int contentColumnHeight(const ScoredStream& s, int cw)
{
    const bool playable = isPlayable(s);
    int h = wrappedHeight(headFont(), headlineOf(s), cw);
    const QString desc = descOf(s);
    if (!desc.isEmpty()) h += kContentGap + wrappedHeight(descFont(), desc, cw);
    if (!streamBadges(s).isEmpty()) h += kContentGap + kBadgeTop + kBadgeH;
    if (!statusOf(s, playable).isEmpty()) { QFontMetrics fm(statusFont()); h += kContentGap + fm.height(); }
    return h;
}

// The real row width comes from the view's viewport (opt.rect is empty during sizeHint).
int rowWidthOf(const QStyleOptionViewItem& opt)
{
    if (const auto* v = qobject_cast<const QAbstractItemView*>(opt.widget))
        return v->viewport()->width();
    return opt.rect.width();
}
} // namespace

SourceRowDelegate::SourceRowDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QSize SourceRowDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    const int w = rowWidthOf(opt);
    const auto s = idx.data(SourceListModel::StreamRole).value<ScoredStream>();
    const int contentH = contentColumnHeight(s, contentWidthFor(w));
    return QSize(w, qMax(kMinRowHeight, contentH + 2 * kPad));
}

QRect SourceRowDelegate::playRect(const QRect& row)
{
    return QRect(row.right() - kPad - kPlaySz, row.center().y() - kPlaySz / 2, kPlaySz, kPlaySz);
}
QRect SourceRowDelegate::copyRect(const QRect& row)
{
    return QRect(row.right() - kPad - kPlaySz - kActionGap - kCopySz, row.center().y() - kCopySz / 2, kCopySz, kCopySz);
}

void SourceRowDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    const auto s = idx.data(SourceListModel::StreamRole).value<ScoredStream>();
    const QString key = streamRowKey(s);
    const bool hovered = key == m_hoveredKey;
    const bool playable = isPlayable(s);
    p->save();
    p->setRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::TextAntialiasing);

    // Card background (rgba(26,29,36,0.40), 1px border, 16px radius). The view's spacing
    // provides the inter-row gap, so the card is the full item rect.
    const QRect card = opt.rect;
    QPainterPath bg; bg.addRoundedRect(card, 16, 16);
    QColor fill = kRowBg; fill.setAlphaF(hovered ? 0.55 : 0.40);
    p->fillPath(bg, fill);
    p->setPen(QPen(QColor(255, 255, 255, 13), 1)); p->drawPath(bg);

    // Logo tile (56x56), centred in the 68px logo column, vertically centred in the row.
    const int tileX = card.left() + kPad + (kLogoCol - kTileSz) / 2;
    QRect tile(tileX, card.center().y() - kTileSz / 2, kTileSz, kTileSz);
    QPainterPath tp; tp.addRoundedRect(tile, 12, 12);
    p->fillPath(tp, kTile);
    p->setPen(QPen(QColor(255, 255, 255, 16), 1)); p->drawPath(tp);
    const QString addon = addonOf(s);
    p->setPen(kMuted);
    p->setFont(tileFont());
    p->drawText(tile, Qt::AlignCenter, addon.trimmed().left(1).toUpper());

    // Content column (word-wrapped headline + desc + badges + status), vertically centred.
    const int cx = card.left() + kPad + kLogoCol + kColGap;
    const int cw = contentWidthFor(card.width());
    const QString head = headlineOf(s);
    const QString desc = descOf(s);
    const QString status = statusOf(s, playable);
    const QStringList badges = streamBadges(s);

    const int headH = wrappedHeight(headFont(), head, cw);
    const int descH = desc.isEmpty() ? 0 : wrappedHeight(descFont(), desc, cw);
    const int badgeBlock = badges.isEmpty() ? 0 : (kBadgeTop + kBadgeH);
    int statusH = 0;
    if (!status.isEmpty()) { QFontMetrics fm(statusFont()); statusH = fm.height(); }
    int blockH = headH;
    if (descH)      blockH += kContentGap + descH;
    if (badgeBlock) blockH += kContentGap + badgeBlock;
    if (statusH)    blockH += kContentGap + statusH;

    int y = card.top() + (card.height() - blockH) / 2;

    // Headline (word-wrapped; Stremio name can be multi-line).
    p->setPen(kHead);
    p->setFont(headFont());
    p->drawText(QRect(cx, y, cw, headH), Qt::TextWordWrap | Qt::AlignTop | Qt::AlignLeft, head);
    y += headH;
    // Description (filename / meta), word-wrapped.
    if (descH) {
        y += kContentGap;
        p->setPen(kMuted);
        p->setFont(descFont());
        p->drawText(QRect(cx, y, cw, descH), Qt::TextWordWrap | Qt::AlignTop | Qt::AlignLeft, desc);
        y += descH;
    }
    // Badge chips.
    if (badgeBlock) {
        y += kContentGap + kBadgeTop;
        p->setFont(badgeFont());
        static const QSet<QString> danger = {"CAM", "TS", "TC", "SCR", "NO LABEL", "UNKNOWN"};
        int bx = cx;
        for (const QString& label : badges) {
            const int w = p->fontMetrics().horizontalAdvance(label) + 14;
            QRect chip(bx, y, w, kBadgeH);
            QPainterPath cp; cp.addRoundedRect(chip, 7, 7);
            p->fillPath(cp, QColor(18, 19, 23, 217));
            const bool d = danger.contains(label);
            if (d) { p->setPen(QPen(QColor(229, 72, 77, 115), 1)); p->drawPath(cp); }
            p->setPen(d ? QColor(0xe5, 0x8a, 0x8d) : kMuted);
            p->drawText(chip, Qt::AlignCenter, label);
            bx += w + 6;
            if (bx > cx + cw - 40) break; // don't overflow toward the actions
        }
        y += kBadgeH;
    }
    // Status line.
    if (statusH) {
        y += kContentGap;
        p->setPen(QColor(0x8b, 0x90, 0x9b));
        p->setFont(statusFont());
        p->drawText(QRect(cx, y, cw, statusH), Qt::AlignVCenter | Qt::AlignLeft, status);
    }

    // Copy button (glyph only), vertically centred.
    const qreal dpr = p->device()->devicePixelRatioF();
    const QRect cr = SourceRowDelegate::copyRect(card);
    const bool hasDirect = !s.url.isEmpty() && s.url != QStringLiteral("#");
    const bool showCopy = hasDirect || !s.externalUrl.isEmpty();
    if (showCopy) {
        const bool copied = key == m_copiedKey;
        const QPixmap g = copied ? glyph("check", kCheckGlyph, kGold, 16, false, dpr)
                                 : glyph("copy", kCopyGlyph, QColor(0x8b, 0x90, 0x9b), 16, false, dpr);
        p->drawPixmap(cr.center() - QPoint(8, 8), g);
    }
    // Play button (64 circle), vertically centred.
    const QRect pr = SourceRowDelegate::playRect(card);
    QPainterPath circ; circ.addEllipse(pr);
    if (playable) { QColor g = kGold; if (hovered) g.setAlphaF(0.90); p->fillPath(circ, g); }
    else { p->fillPath(circ, QColor(26, 29, 36, 179)); p->setPen(QPen(QColor(255, 255, 255, 16), 1)); p->drawPath(circ); }
    const QPixmap pg = glyph("play", kPlayGlyph, playable ? kInk : QColor(0x8b, 0x90, 0x9b), 26, true, dpr);
    p->drawPixmap(pr.center() - QPoint(13, 13), pg);

    p->restore();
}

} // namespace tankoban
