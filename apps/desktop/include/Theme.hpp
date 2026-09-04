#pragma once

#include <QApplication>
#include <QColor>
#include <QString>

namespace Pocket::App::Theme {

QColor surface();
QColor surfaceRaised();
// Translucent layers. Widgets composite over the window background, so these are
// real alpha, not a precomputed blend, and stay correct if the window ever changes.
QColor surfacePanel();     // sidebar, toolbar, inspector
QColor surfaceControl();   // inputs, buttons
QColor surfaceHover();
QColor borderSubtle();
QColor borderHover();
// CSS "rgba(r, g, b, a)" for stylesheets, which cannot take a QColor.
QString rgba(const QColor& color);
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
