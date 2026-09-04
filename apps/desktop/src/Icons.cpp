#include "Icons.hpp"

#include <QPainter>
#include <QPixmapCache>
#include <QSvgRenderer>

namespace {

using Pocket::App::Icons::Name;

// 24x24 viewBox, 1.8 stroke, round joins: one geometry for the whole app.
QString body(Name name)
{
    switch (name) {
    case Name::Search:
        return R"(<circle cx="11" cy="11" r="7"/><path d="M20.5 20.5 L16 16"/>)";
    case Name::Clock:
        return R"(<circle cx="12" cy="12" r="9"/><path d="M12 6.5 V12 l4 2.2"/>)";
    case Name::Grid:
        return R"(<rect x="3.5" y="3.5" width="7" height="7" rx="1.5"/><rect x="13.5" y="3.5" width="7" height="7" rx="1.5"/>)"
               R"(<rect x="3.5" y="13.5" width="7" height="7" rx="1.5"/><rect x="13.5" y="13.5" width="7" height="7" rx="1.5"/>)";
    case Name::GridSmall:
        return R"(<rect x="3.5" y="3.5" width="4.2" height="4.2" rx="1"/><rect x="9.9" y="3.5" width="4.2" height="4.2" rx="1"/><rect x="16.3" y="3.5" width="4.2" height="4.2" rx="1"/>)"
               R"(<rect x="3.5" y="9.9" width="4.2" height="4.2" rx="1"/><rect x="9.9" y="9.9" width="4.2" height="4.2" rx="1"/><rect x="16.3" y="9.9" width="4.2" height="4.2" rx="1"/>)"
               R"(<rect x="3.5" y="16.3" width="4.2" height="4.2" rx="1"/><rect x="9.9" y="16.3" width="4.2" height="4.2" rx="1"/><rect x="16.3" y="16.3" width="4.2" height="4.2" rx="1"/>)";
    case Name::GridLarge:
        return R"(<rect x="3.5" y="3.5" width="8" height="8" rx="1.5"/><rect x="12.5" y="3.5" width="8" height="8" rx="1.5"/>)"
               R"(<rect x="3.5" y="12.5" width="8" height="8" rx="1.5"/><rect x="12.5" y="12.5" width="8" height="8" rx="1.5"/>)";
    case Name::Sort:
        return R"(<path d="M7.5 3.5 V20"/><path d="M4 7 L7.5 3.5 L11 7"/><path d="M16.5 20.5 V4"/><path d="M13 17 L16.5 20.5 L20 17"/>)";
    case Name::Plus:
        return R"(<path d="M12 5 V19"/><path d="M5 12 H19"/>)";
    case Name::Play:
        return R"(<path d="M7 4.5 L19 12 L7 19.5 Z" stroke-linejoin="round"/>)";
    case Name::Settings:
        return R"(<path d="M4 7 H20"/><path d="M4 12 H20"/><path d="M4 17 H20"/><circle cx="9" cy="7" r="2"/><circle cx="15" cy="12" r="2"/><circle cx="9" cy="17" r="2"/>)";
    case Name::More:
        return R"(<circle cx="5.5" cy="12" r="1.4"/><circle cx="12" cy="12" r="1.4"/><circle cx="18.5" cy="12" r="1.4"/>)";
    case Name::Folder:
        return R"(<path d="M3.5 6.5 h5.5 l2 2.2 h9.5 v10.8 h-17 Z"/>)";
    case Name::Image:
        return R"(<rect x="3.5" y="4.5" width="17" height="15" rx="2"/><circle cx="9" cy="10" r="1.6"/><path d="M4.5 17.5 L10 12 L20 19"/>)";
    case Name::Info:
        return R"(<circle cx="12" cy="12" r="9"/><path d="M12 11 V16.5"/><circle cx="12" cy="7.8" r="0.9"/>)";
    case Name::Trash:
        return R"(<path d="M4 6.5 H20"/><path d="M9.5 6.5 V4.5 h5 v2"/><path d="M6.5 6.5 L7.5 20 h9 l1 -13.5"/>)";
    }
    return {};
}

bool filled(Name name)
{
    return name == Name::Play || name == Name::More;
}

} // namespace

namespace Pocket::App::Icons {

QPixmap pixmap(Name name, const QColor& color, int size)
{
    const QString key = QStringLiteral("pp:icon:%1:%2:%3").arg(static_cast<int>(name)).arg(color.name(QColor::HexArgb)).arg(size);
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) return cached;

    const QString fill = filled(name) ? color.name() : QStringLiteral("none");
    const QString svg = QStringLiteral(
        R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="%1" stroke="%2" )"
        R"(stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">%3</svg>)")
        .arg(fill, color.name(), body(name));

    QSvgRenderer renderer(svg.toUtf8());
    QPixmap result(size, size);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter, QRectF(0, 0, size, size));
    painter.end();
    if (color.alphaF() < 1.0) {
        // The SVG colour string drops alpha, so apply it once over the raster.
        QPixmap faded(size, size);
        faded.fill(Qt::transparent);
        QPainter fadePainter(&faded);
        fadePainter.setOpacity(color.alphaF());
        fadePainter.drawPixmap(0, 0, result);
        fadePainter.end();
        result = faded;
    }
    QPixmapCache::insert(key, result);
    return result;
}

QIcon icon(Name name, const QColor& color, int size)
{
    return QIcon(pixmap(name, color, size));
}

} // namespace Pocket::App::Icons
