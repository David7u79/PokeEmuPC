#pragma once

#include <QIcon>
#include <QPixmap>

namespace Pocket::App::Icons {

// One family, one stroke width, drawn from inline SVG so there is no icon
// dependency and no .qrc to keep in sync.
enum class Name {
    Search,
    Clock,
    Grid,
    GridSmall,
    GridLarge,
    Sort,
    Plus,
    Play,
    Settings,
    More,
    Folder,
    Image,
    Info,
    Trash
};

QPixmap pixmap(Name name, const QColor& color, int size = 16);
QIcon icon(Name name, const QColor& color, int size = 16);

} // namespace Pocket::App::Icons
