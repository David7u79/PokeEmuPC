#include "Theme.hpp"
#include <QPalette>

namespace Pocket::App::Theme {

QColor surface() {
    return QColor("#14161a");
}

QColor surfaceRaised() {
    return QColor("#1f232a");
}

QColor accent() {
    return QColor("#4f8cff");
}

QColor accentPressed() {
    return QColor("#3a74e0");
}

QColor textPrimary() {
    return QColor("#e8ecf2");
}

QColor textSecondary() {
    return QColor("#9aa4b2");
}

QColor textDisabled() {
    return QColor("#5b6472");
}

QColor border() {
    return QColor("#2a2f38");
}

void applyTheme(QApplication& app) {
    QPalette pal;
    const QColor colSurface = surface();
    const QColor colSurfaceRaised = surfaceRaised();
    const QColor colBase("#191c21");
    const QColor colPanel("#1b1f25");
    const QColor colTextPrimary = textPrimary();
    const QColor colTextSecondary = textSecondary();
    const QColor colTextDisabled = textDisabled();
    const QColor colAccent = accent();
    const QColor colBorder = border();

    pal.setColor(QPalette::Window, colSurface);
    pal.setColor(QPalette::WindowText, colTextPrimary);
    pal.setColor(QPalette::Base, colBase);
    pal.setColor(QPalette::AlternateBase, colPanel);
    pal.setColor(QPalette::ToolTipBase, colSurfaceRaised);
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

    const QString styleSheet = QStringLiteral(
        "QWidget {\n"
        "    background-color: #14161a;\n"
        "    color: #e8ecf2;\n"
        "    font-size: 13px;\n"
        "}\n"
        "QMainWindow {\n"
        "    background-color: #14161a;\n"
        "}\n"
        "QWidget#appNavigation {\n"
        "    background-color: #191c21;\n"
        "    border-bottom: 1px solid #2a2f38;\n"
        "}\n"
        "QLabel#navBrand {\n"
        "    color: #9aa4b2;\n"
        "    font-size: 14px;\n"
        "    font-weight: bold;\n"
        "    background: transparent;\n"
        "    padding-left: 4px;\n"
        "}\n"
        "QPushButton#navLibrary, QPushButton#navCompanion {\n"
        "    background-color: transparent;\n"
        "    color: #9aa4b2;\n"
        "    border: 1px solid transparent;\n"
        "    border-bottom: 2px solid transparent;\n"
        "    padding: 8px 18px;\n"
        "    font-size: 13px;\n"
        "    font-weight: 600;\n"
        "    border-radius: 0px;\n"
        "}\n"
        "QPushButton#navLibrary:hover, QPushButton#navCompanion:hover {\n"
        "    color: #e8ecf2;\n"
        "    background-color: #1f232a;\n"
        "    border: 1px solid #2a2f38;\n"
        "    border-bottom: 2px solid transparent;\n"
        "}\n"
        "QPushButton#navLibrary:focus, QPushButton#navCompanion:focus {\n"
        "    color: #e8ecf2;\n"
        "    border: 1px solid #4f8cff;\n"
        "    border-bottom: 2px solid #4f8cff;\n"
        "}\n"
        "QPushButton#navLibrary[active=\"true\"], QPushButton#navCompanion[active=\"true\"] {\n"
        "    color: #e8ecf2;\n"
        "    border: 1px solid transparent;\n"
        "    border-bottom: 2px solid #4f8cff;\n"
        "    background-color: #1f232a;\n"
        "}\n"
        "QPushButton#navSettings {\n"
        "    background-color: #1f232a;\n"
        "    color: #9aa4b2;\n"
        "    border: 1px solid #2a2f38;\n"
        "    border-radius: 6px;\n"
        "    padding: 6px 14px;\n"
        "    font-size: 13px;\n"
        "}\n"
        "QPushButton#navSettings:hover {\n"
        "    background-color: #262c36;\n"
        "    color: #e8ecf2;\n"
        "    border: 1px solid #4f8cff;\n"
        "}\n"
        "QPushButton#navSettings:pressed {\n"
        "    background-color: #191c21;\n"
        "    border: 1px solid #3a74e0;\n"
        "}\n"
        "QPushButton#navSettings:focus {\n"
        "    border: 1px solid #4f8cff;\n"
        "    color: #e8ecf2;\n"
        "}\n"
        "QPushButton#navSettings[active=\"true\"] {\n"
        "    background-color: #262c36;\n"
        "    color: #e8ecf2;\n"
        "    border: 1px solid #4f8cff;\n"
        "}\n"
        "QPushButton, QToolButton {\n"
        "    background-color: #1f232a;\n"
        "    color: #e8ecf2;\n"
        "    border: 1px solid #2a2f38;\n"
        "    border-radius: 6px;\n"
        "    padding: 5px 12px;\n"
        "}\n"
        "QPushButton:hover, QToolButton:hover {\n"
        "    background-color: #262c36;\n"
        "    border: 1px solid #4f8cff;\n"
        "}\n"
        "QPushButton:pressed, QToolButton:pressed {\n"
        "    background-color: #191c21;\n"
        "    border: 1px solid #3a74e0;\n"
        "}\n"
        "QPushButton:focus, QToolButton:focus {\n"
        "    border: 1px solid #4f8cff;\n"
        "}\n"
        "QPushButton:disabled, QToolButton:disabled {\n"
        "    background-color: #191c21;\n"
        "    color: #5b6472;\n"
        "    border: 1px solid #22262e;\n"
        "}\n"
        "QPushButton#backToLibraryButton {\n"
        "    background-color: #1f232a;\n"
        "    color: #e8ecf2;\n"
        "    border: 1px solid #2a2f38;\n"
        "    border-radius: 6px;\n"
        "    padding: 4px 12px;\n"
        "    font-weight: 600;\n"
        "}\n"
        "QPushButton#backToLibraryButton:hover {\n"
        "    background-color: #262c36;\n"
        "    border: 1px solid #4f8cff;\n"
        "}\n"
        "QPushButton#backToLibraryButton:pressed {\n"
        "    background-color: #191c21;\n"
        "    border: 1px solid #3a74e0;\n"
        "}\n"
        "QPushButton#backToLibraryButton:focus {\n"
        "    border: 1px solid #4f8cff;\n"
        "}\n"
        "QLineEdit, QComboBox, QSpinBox {\n"
        "    background-color: #191c21;\n"
        "    color: #e8ecf2;\n"
        "    border: 1px solid #2a2f38;\n"
        "    border-radius: 6px;\n"
        "    padding: 5px 8px;\n"
        "}\n"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus {\n"
        "    border: 1px solid #4f8cff;\n"
        "    background-color: #1b1f25;\n"
        "}\n"
        "QTabWidget::pane {\n"
        "    border: 1px solid #2a2f38;\n"
        "    background-color: #14161a;\n"
        "    border-radius: 6px;\n"
        "}\n"
        "QTabBar::tab {\n"
        "    background-color: #191c21;\n"
        "    color: #9aa4b2;\n"
        "    border: 1px solid #2a2f38;\n"
        "    border-bottom: none;\n"
        "    border-top-left-radius: 6px;\n"
        "    border-top-right-radius: 6px;\n"
        "    padding: 6px 14px;\n"
        "    margin-right: 2px;\n"
        "}\n"
        "QTabBar::tab:selected {\n"
        "    background-color: #1f232a;\n"
        "    color: #e8ecf2;\n"
        "    border-bottom: 2px solid #4f8cff;\n"
        "}\n"
        "QTabBar::tab:hover:!selected {\n"
        "    background-color: #22262e;\n"
        "    color: #e8ecf2;\n"
        "}\n"
        "QScrollBar:vertical {\n"
        "    background-color: #14161a;\n"
        "    width: 10px;\n"
        "    margin: 0px;\n"
        "}\n"
        "QScrollBar::handle:vertical {\n"
        "    background-color: #2a2f38;\n"
        "    min-height: 20px;\n"
        "    border-radius: 5px;\n"
        "}\n"
        "QScrollBar::handle:vertical:hover {\n"
        "    background-color: #3a414e;\n"
        "}\n"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {\n"
        "    height: 0px;\n"
        "}\n"
        "QGroupBox {\n"
        "    border: 1px solid #2a2f38;\n"
        "    border-radius: 6px;\n"
        "    margin-top: 14px;\n"
        "    padding-top: 12px;\n"
        "    font-weight: bold;\n"
        "    color: #e8ecf2;\n"
        "}\n"
        "QGroupBox::title {\n"
        "    subcontrol-origin: margin;\n"
        "    subcontrol-position: top left;\n"
        "    left: 10px;\n"
        "    padding: 0 4px;\n"
        "}\n"
        "QMenu {\n"
        "    background-color: #1f232a;\n"
        "    border: 1px solid #2a2f38;\n"
        "    border-radius: 6px;\n"
        "    padding: 4px;\n"
        "}\n"
        "QMenu::item {\n"
        "    padding: 6px 20px;\n"
        "    border-radius: 4px;\n"
        "    color: #e8ecf2;\n"
        "}\n"
        "QMenu::item:selected {\n"
        "    background-color: #262c36;\n"
        "    border: 1px solid #4f8cff;\n"
        "}\n"
        "QMenu::separator {\n"
        "    height: 1px;\n"
        "    background-color: #2a2f38;\n"
        "    margin: 4px 0px;\n"
        "}\n"
    );

    app.setStyleSheet(styleSheet);
}

} // namespace Pocket::App::Theme
