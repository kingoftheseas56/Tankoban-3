// Tankoban 3 - see SourceRowDelegate.h.
#include "ui/SourceRowDelegate.h"
#include "ui/SourceListModel.h"
#include "ui/StreamRowPaint.h"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPixmapCache>
#include <QSvgRenderer>
#include <QSet>

namespace tankoban {
namespace {
const QColor kRowBg(26, 29, 36);          // a=0.40 applied at fill
const QColor kTile(26, 29, 36);
const QColor kHead(0xf3, 0xf1, 0xea);
const QColor kMuted(0xaa, 0xb1, 0xbd);
const QColor kGold(0xe8, 0xb9, 0x23);
const QColor kInk(0x12, 0x13, 0x17);

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
} // namespace

SourceRowDelegate::SourceRowDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QSize SourceRowDelegate::sizeHint(const QStyleOptionViewItem& opt, const QModelIndex&) const
{
    return QSize(opt.rect.width(), kRowHeight);
}

QRect SourceRowDelegate::playRect(const QRect& row)
{
    return QRect(row.right() - 20 - 64, row.center().y() - 32, 64, 64);
}
QRect SourceRowDelegate::copyRect(const QRect& row)
{
    return QRect(row.right() - 20 - 64 - 8 - 36, row.center().y() - 18, 36, 36);
}

void SourceRowDelegate::paint(QPainter* p, const QStyleOptionViewItem& opt, const QModelIndex& idx) const
{
    const auto s = idx.data(SourceListModel::StreamRole).value<ScoredStream>();
    const QString key = streamRowKey(s);
    const bool hovered = key == m_hoveredKey;
    const bool playable = isPlayable(s);
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    // Card background (rgba(26,29,36,0.40), 1px border, 16px radius) inset by ~no extra
    // margin — the view's spacing provides the gap.
    QRect card = opt.rect.adjusted(0, 4, 0, -4);
    QPainterPath bg; bg.addRoundedRect(card, 16, 16);
    QColor fill = kRowBg; fill.setAlphaF(hovered ? 0.55 : 0.40);
    p->fillPath(bg, fill);
    p->setPen(QPen(QColor(255, 255, 255, 13), 1)); p->drawPath(bg);

    const int left = card.left() + 20;
    // Logo tile 56x56.
    QRect tile(left, card.center().y() - 28, 56, 56);
    QPainterPath tp; tp.addRoundedRect(tile, 12, 12);
    p->fillPath(tp, kTile);
    p->setPen(QPen(QColor(255,255,255,16),1)); p->drawPath(tp);
    const QString addon = !s.addonName.isEmpty() ? s.addonName : s.addonId;
    p->setPen(kMuted);
    { QFont f("Inter"); f.setPixelSize(22); f.setWeight(QFont::Bold); p->setFont(f); }
    p->drawText(tile, Qt::AlignCenter, addon.trimmed().left(1).toUpper());

    // Content column.
    const int cx = left + 56 + 20;
    const int cw = SourceRowDelegate::copyRect(card).left() - 12 - cx;
    int y = card.top() + 20;
    // Headline.
    p->setPen(kHead);
    { QFont f("Inter"); f.setPixelSize(16); f.setWeight(QFont::DemiBold); p->setFont(f); }
    const QString head = !s.name.trimmed().isEmpty() ? s.name.trimmed() : addon;
    p->drawText(QRect(cx, y, cw, 22), Qt::AlignVCenter | Qt::AlignLeft,
                p->fontMetrics().elidedText(head, Qt::ElideRight, cw));
    y += 24;
    // Description (filename), elided to one line.
    const QString desc = !s.title.trimmed().isEmpty() ? s.title.trimmed() : s.description.trimmed();
    if (!desc.isEmpty()) {
        p->setPen(kMuted);
        { QFont f("Inter"); f.setPixelSize(14); p->setFont(f); }
        p->drawText(QRect(cx, y, cw, 20), Qt::AlignVCenter | Qt::AlignLeft,
                    p->fontMetrics().elidedText(desc, Qt::ElideRight, cw));
        y += 22;
    }
    // Badge chips.
    int bx = cx;
    { QFont f("Inter"); f.setPixelSize(11); f.setWeight(QFont::DemiBold); p->setFont(f); }
    static const QSet<QString> danger = {"CAM","TS","TC","SCR","NO LABEL","UNKNOWN"};
    for (const QString& label : streamBadges(s)) {
        const int w = p->fontMetrics().horizontalAdvance(label) + 14;
        QRect chip(bx, y, w, 18);
        QPainterPath cp; cp.addRoundedRect(chip, 7, 7);
        p->fillPath(cp, QColor(18,19,23,217));
        const bool d = danger.contains(label);
        p->setPen(d ? QColor(0xe5,0x8a,0x8d) : kMuted);
        if (d) { p->setPen(QPen(QColor(229,72,77,115),1)); p->drawPath(cp); p->setPen(QColor(0xe5,0x8a,0x8d)); }
        p->drawText(chip, Qt::AlignCenter, label);
        bx += w + 6;
        if (bx > cx + cw - 40) break;   // don't overflow into actions
    }
    y += 22;
    // Status line.
    QString status;
    const bool isTorrent = !s.infoHash.isEmpty();
    const bool hasDirect = !s.url.isEmpty() && s.url != QStringLiteral("#");
    if (isTorrent && !hasDirect) status = QStringLiteral("Torrent · P2P stream");
    else if (!playable && !s.externalUrl.isEmpty()) status = QStringLiteral("External source");
    if (!status.isEmpty()) {
        p->setPen(QColor(0x8b,0x90,0x9b));
        { QFont f("Inter"); f.setPixelSize(13); f.setWeight(QFont::DemiBold); p->setFont(f); }
        p->drawText(QRect(cx, y, cw, 16), Qt::AlignVCenter | Qt::AlignLeft, status);
    }

    // Copy button (glyph only).
    const qreal dpr = p->device()->devicePixelRatioF();
    const QRect cr = SourceRowDelegate::copyRect(card);
    const bool showCopy = hasDirect || !s.externalUrl.isEmpty();
    if (showCopy) {
        const bool copied = key == m_copiedKey;
        const QPixmap g = copied ? glyph("check", kCheckGlyph, kGold, 16, false, dpr)
                                 : glyph("copy", kCopyGlyph, QColor(0x8b,0x90,0x9b), 16, false, dpr);
        p->drawPixmap(cr.center() - QPoint(8,8), g);
    }
    // Play button (64 circle).
    const QRect pr = SourceRowDelegate::playRect(card);
    QPainterPath circ; circ.addEllipse(pr);
    if (playable) { QColor g = kGold; if (hovered) g.setAlphaF(0.90); p->fillPath(circ, g); }
    else { p->fillPath(circ, QColor(26,29,36,179)); p->setPen(QPen(QColor(255,255,255,16),1)); p->drawPath(circ); }
    const QPixmap pg = glyph("play", kPlayGlyph, playable ? kInk : QColor(0x8b,0x90,0x9b), 26, true, dpr);
    p->drawPixmap(pr.center() - QPoint(13,13), pg);

    p->restore();
}

} // namespace tankoban
