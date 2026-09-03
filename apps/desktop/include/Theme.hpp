#pragma once

#include <QApplication>
#include <QColor>

namespace Pocket::App::Theme {

QColor surface();
QColor surfaceRaised();
QColor accent();
QColor accentPressed();
QColor textPrimary();
QColor textSecondary();
QColor textDisabled();
QColor border();

void applyTheme(QApplication& app);

} // namespace Pocket::App::Theme

namespace Pocket::App {
using Theme::applyTheme;
}
