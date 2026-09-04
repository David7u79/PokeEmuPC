#include "Theme.hpp"
#include <QPalette>

namespace Pocket::App::Theme {

QColor surface() {
    // Near-black with a touch of blue: pure black flattens everything on it.
    return QColor("#0b0d10");
}

QColor surfaceRaised() {
    return QColor("#1f232a");
}

QColor surfacePanel() {
    return QColor(20, 23, 28, 184);   // 0.72
}

QColor surfaceControl() {
    return QColor(28, 32, 39, 158);   // 0.62
}

QColor surfaceHover() {
    return QColor(255, 255, 255, 14); // 0.055
}

QColor borderSubtle() {
    return QColor(255, 255, 255, 18); // 0.07
}

QColor borderHover() {
    return QColor(255, 255, 255, 31); // 0.12
}

QString rgba(const QColor& color) {
    return QStringLiteral("rgba(%1, %2, %3, %4)")
        .arg(color.red()).arg(color.green()).arg(color.blue())
        .arg(QString::number(color.alphaF(), 'f', 3));
}

QColor accent() {
    return QColor("#4f8cff");
}

QColor accentPressed() {
    return QColor("#3a74e0");
}

QColor textPrimary() {
    return QColor("#f0f1f4");
}

QColor textSecondary() {
    return QColor("#9ba1ab");
}

QColor textDisabled() {
    return QColor("#6c727c");
}

QColor border() {
    return QColor("#2a2f38");
}

void applyTheme(QApplication& app) {
    QPalette pal;
    const QColor colSurface = surface();
    const QColor colSurfaceRaised = surfaceRaised();
    const QColor colBase("#14171c");
    const QColor colPanel("#171a20");
    const QColor colTextPrimary = textPrimary();
    const QColor colTextSecondary = textSecondary();
    const QColor colTextDisabled = textDisabled();
    const QColor colAccent = accent();
    const QColor colBorder = border();

    pal.setColor(QPalette::Window, colSurface);
    pal.setColor(QPalette::WindowText, colTextPrimary);
    pal.setColor(QPalette::Base, colBase);
    pal.setColor(QPalette::AlternateBase, colPanel);
    pal.setColor(QPalette::ToolTipBase, colPanel);
    pal.setColor(QPalette::ToolTipText, colTextPrimary);
    pal.setColor(QPalette::Text, colTextPrimary);
    pal.setColor(QPalette::Button, colSurfaceRaised);
    pal.setColor(QPalette::ButtonText, colTextPrimary);
    pal.setColor(QPalette::BrightText, Qt::white);
    pal.setColor(QPalette::Link, colAccent);
    pal.setColor(QPalette::Highlight, colAccent);
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::PlaceholderText, colTextSecondary);

    pal.setColor(QPalette::Disabled, QPalette::WindowText, colTextDisabled);
    pal.setColor(QPalette::Disabled, QPalette::Text, colTextDisabled);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, colTextDisabled);
    pal.setColor(QPalette::Disabled, QPalette::Highlight, colBorder);

    app.setPalette(pal);

    // One place for the surface tokens. Everything below is written in terms of
    // them, so a colour is changed here and nowhere else.
    const QString appBg = colSurface.name();
    const QString panel = rgba(surfacePanel());
    const QString control = rgba(surfaceControl());
    const QString hover = rgba(surfaceHover());
    const QString borderLine = rgba(borderSubtle());
    const QString borderStrong = rgba(borderHover());
    const QString primary = colTextPrimary.name();
    const QString secondary = colTextSecondary.name();
    const QString disabled = colTextDisabled.name();
    const QString accentColor = colAccent.name();
    // Menus and tooltips are their own top-level windows: alpha there would
    // composite against nothing, so they stay solid.
    const QString popup = colPanel.name();

    const QString styleSheet =
        "QWidget { background-color: " + appBg + "; color: " + primary + "; font-size: 13px; }\n"
        "QMainWindow { background-color: " + appBg + "; }\n"

        "QWidget#appNavigation { background-color: " + panel + "; border-bottom: 1px solid " + borderLine + "; }\n"
        "QLabel#navBrand { color: " + secondary + "; font-size: 14px; font-weight: 600; background: transparent; padding-left: 4px; }\n"
        "QPushButton#navLibrary, QPushButton#navCompanion {\n"
        "    background-color: transparent; color: " + secondary + ";\n"
        "    border: none; border-bottom: 2px solid transparent;\n"
        "    padding: 8px 18px; font-size: 13px; font-weight: 600; border-radius: 0px;\n"
        "}\n"
        "QPushButton#navLibrary:hover, QPushButton#navCompanion:hover { color: " + primary + "; background-color: " + hover + "; }\n"
        "QPushButton#navLibrary:focus, QPushButton#navCompanion:focus { color: " + primary + "; border-bottom: 2px solid " + accentColor + "; }\n"
        "QPushButton#navLibrary[active=\"true\"], QPushButton#navCompanion[active=\"true\"] {\n"
        "    color: " + primary + "; background-color: transparent; border-bottom: 2px solid " + accentColor + ";\n"
        "}\n"
        "QPushButton#navSettings, QPushButton#navResume {\n"
        "    background-color: " + control + "; color: " + secondary + ";\n"
        "    border: 1px solid " + borderLine + "; border-radius: 6px; padding: 5px 12px; font-size: 13px;\n"
        "}\n"
        "QPushButton#navSettings:hover, QPushButton#navResume:hover { background-color: " + hover + "; color: " + primary + "; border: 1px solid " + borderStrong + "; }\n"
        "QPushButton#navSettings:focus, QPushButton#navResume:focus { border: 1px solid " + accentColor + "; color: " + primary + "; }\n"
        "QPushButton#navSettings[active=\"true\"] { color: " + primary + "; border: 1px solid " + borderStrong + "; }\n"

        "QPushButton, QToolButton {\n"
        "    background-color: " + control + "; color: " + primary + ";\n"
        "    border: 1px solid " + borderLine + "; border-radius: 6px; padding: 5px 12px;\n"
        "}\n"
        "QPushButton:hover, QToolButton:hover { background-color: " + hover + "; border: 1px solid " + borderStrong + "; }\n"
        "QPushButton:pressed, QToolButton:pressed { background-color: rgba(0, 0, 0, 0.25); }\n"
        "QPushButton:focus, QToolButton:focus { border: 1px solid " + accentColor + "; }\n"
        "QPushButton:disabled, QToolButton:disabled { background-color: transparent; color: " + disabled + "; border: 1px solid " + borderLine + "; }\n"
        "QPushButton#backToLibraryButton { background-color: " + control + "; border: 1px solid " + borderLine + "; font-weight: 600; }\n"

        "QLineEdit, QComboBox, QSpinBox {\n"
        "    background-color: " + control + "; color: " + primary + ";\n"
        "    border: 1px solid " + borderLine + "; border-radius: 6px; padding: 6px 10px;\n"
        "    selection-background-color: " + accentColor + ";\n"
        "}\n"
        "QLineEdit:hover, QComboBox:hover, QSpinBox:hover { border: 1px solid " + borderStrong + "; }\n"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border: 1px solid " + accentColor + "; }\n"
        "QComboBox::drop-down { border: none; width: 18px; }\n"
        "QComboBox QAbstractItemView { background-color: " + popup + "; border: 1px solid " + borderLine + "; selection-background-color: " + hover + "; outline: none; }\n"

        // Thin, trackless: the library scrolls without a chrome bar down its side.
        "QScrollBar:vertical { background: transparent; width: 8px; margin: 2px 0px; }\n"
        "QScrollBar::handle:vertical { background-color: rgba(255, 255, 255, 0.12); min-height: 28px; border-radius: 4px; }\n"
        "QScrollBar::handle:vertical:hover { background-color: rgba(255, 255, 255, 0.22); }\n"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }\n"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }\n"
        "QScrollBar:horizontal { background: transparent; height: 8px; margin: 0px 2px; }\n"
        "QScrollBar::handle:horizontal { background-color: rgba(255, 255, 255, 0.12); min-width: 28px; border-radius: 4px; }\n"
        "QScrollBar::handle:horizontal:hover { background-color: rgba(255, 255, 255, 0.22); }\n"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }\n"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }\n"

        "QSlider { background: transparent; }\n"
        "QSlider::groove:horizontal { height: 3px; background: rgba(255, 255, 255, 0.16); border-radius: 2px; margin: 0px; }\n"
        "QSlider::handle:horizontal { background: " + primary + "; width: 11px; margin: -5px 0px; border-radius: 5px; }\n"
        "QSlider::handle:horizontal:hover { background: #ffffff; }\n"

        "QTabWidget::pane { border: none; background-color: transparent; }\n"
        "QTabBar::tab {\n"
        "    background-color: transparent; color: " + secondary + ";\n"
        "    border: none; border-bottom: 2px solid transparent; padding: 7px 14px; margin-right: 4px;\n"
        "}\n"
        "QTabBar::tab:selected { color: " + primary + "; border-bottom: 2px solid " + accentColor + "; }\n"
        "QTabBar::tab:hover:!selected { color: " + primary + "; background-color: " + hover + "; }\n"

        "QGroupBox { border: 1px solid " + borderLine + "; border-radius: 8px; margin-top: 14px; padding-top: 12px; font-weight: 600; color: " + primary + "; }\n"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 4px; }\n"

        "QMenu { background-color: " + popup + "; border: 1px solid " + borderLine + "; border-radius: 8px; padding: 5px; }\n"
        "QMenu::item { padding: 6px 22px 6px 12px; border-radius: 5px; color: " + primary + "; }\n"
        "QMenu::item:selected { background-color: " + hover + "; }\n"
        "QMenu::item:disabled { color: " + disabled + "; }\n"
        "QMenu::separator { height: 1px; background-color: " + borderLine + "; margin: 5px 8px; }\n"
        "QToolTip { background-color: " + popup + "; color: " + primary + "; border: 1px solid " + borderLine + "; padding: 4px 7px; border-radius: 5px; }\n";

    app.setStyleSheet(styleSheet);
}

} // namespace Pocket::App::Theme
