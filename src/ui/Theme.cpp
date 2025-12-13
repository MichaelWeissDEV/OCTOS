# 1 "./src/ui/Theme.cpp"
#include "Theme.h"

#include <QStringList>

QString Theme::getDarkStylesheet() {
    return R"(
        QWidget {
            background-color: #1e1e1e;
            color: #e0e0e0;
        }
        
        QLineEdit, QPlainTextEdit, QComboBox {
            background-color: #252526;
            color: #e0e0e0;
            border: 1px solid #3e3e42;
            padding: 4px;
            border-radius: 3px;
        }
        
        QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus {
            border: 1px solid #007acc;
        }
        
        QPushButton {
            background-color: #007acc;
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #005a9e;
        }
        
        QPushButton:pressed {
            background-color: #004578;
        }
        
        QLabel {
            color: #e0e0e0;
        }
        
        QSplitter::handle {
            background-color: #3e3e42;
        }
        
        QSplitter::handle:hover {
            background-color: #007acc;
        }
        
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        
        QComboBox::down-arrow {
            image: none;
            width: 8px;
            height: 8px;
        }
        
        QAbstractItemView {
            background-color: #252526;
            color: #e0e0e0;
            selection-background-color: #007acc;
            border: 1px solid #3e3e42;
        }
    )";
}

QString Theme::getLightStylesheet() {
    return R"(
        QWidget {
            background-color: #ffffff;
            color: #000000;
        }
        
        QLineEdit, QPlainTextEdit, QComboBox {
            background-color: #f5f5f5;
            color: #000000;
            border: 1px solid #d0d0d0;
            padding: 4px;
            border-radius: 3px;
        }
        
        QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus {
            border: 1px solid #0066cc;
        }
        
        QPushButton {
            background-color: #0066cc;
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #0052a3;
        }
        
        QPushButton:pressed {
            background-color: #003d7a;
        }
        
        QLabel {
            color: #000000;
        }
        
        QSplitter::handle {
            background-color: #d0d0d0;
        }
        
        QSplitter::handle:hover {
            background-color: #0066cc;
        }
        
        QAbstractItemView {
            background-color: #ffffff;
            color: #000000;
            selection-background-color: #0066cc;
            border: 1px solid #d0d0d0;
        }
    )";
}

QString Theme::getSolarizedDarkStylesheet() {
    return R"(
        QWidget {
            background-color: #002b36;
            color: #839496;
        }
        
        QLineEdit, QPlainTextEdit, QComboBox {
            background-color: #073642;
            color: #839496;
            border: 1px solid #586e75;
            padding: 4px;
            border-radius: 3px;
        }
        
        QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus {
            border: 1px solid #268bd2;
        }
        
        QPushButton {
            background-color: #268bd2;
            color: #002b36;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #2aa198;
        }
        
        QLabel {
            color: #839496;
        }
        
        QAbstractItemView {
            background-color: #073642;
            color: #839496;
            selection-background-color: #268bd2;
            border: 1px solid #586e75;
        }
    )";
}

QString Theme::getSolarizedLightStylesheet() {
    return R"(
        QWidget {
            background-color: #fdf6e3;
            color: #657b83;
        }
        
        QLineEdit, QPlainTextEdit, QComboBox {
            background-color: #eee8d5;
            color: #657b83;
            border: 1px solid #d6d0c8;
            padding: 4px;
            border-radius: 3px;
        }
        
        QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus {
            border: 1px solid #268bd2;
        }
        
        QPushButton {
            background-color: #268bd2;
            color: #fdf6e3;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #2aa198;
        }
        
        QLabel {
            color: #657b83;
        }
        
        QAbstractItemView {
            background-color: #eee8d5;
            color: #657b83;
            selection-background-color: #268bd2;
            border: 1px solid #d6d0c8;
        }
    )";
}

QString Theme::getDraculaStylesheet() {
    return R"(
        QWidget {
            background-color: #282a36;
            color: #f8f8f2;
        }
        
        QLineEdit, QPlainTextEdit, QComboBox {
            background-color: #44475a;
            color: #f8f8f2;
            border: 1px solid #6272a4;
            padding: 4px;
            border-radius: 3px;
        }
        
        QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus {
            border: 1px solid #bd93f9;
        }
        
        QPushButton {
            background-color: #bd93f9;
            color: #282a36;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #8be9fd;
        }
        
        QLabel {
            color: #f8f8f2;
        }
        
        QAbstractItemView {
            background-color: #44475a;
            color: #f8f8f2;
            selection-background-color: #bd93f9;
            border: 1px solid #6272a4;
        }
    )";
}

QString Theme::getNordStylesheet() {
    return R"(
        QWidget {
            background-color: #2e3440;
            color: #d8dee9;
        }
        
        QLineEdit, QPlainTextEdit, QComboBox {
            background-color: #3b4252;
            color: #d8dee9;
            border: 1px solid #434c5e;
            padding: 4px;
            border-radius: 3px;
        }
        
        QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus {
            border: 1px solid #88c0d0;
        }
        
        QPushButton {
            background-color: #88c0d0;
            color: #2e3440;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #81a1c1;
        }
        
        QLabel {
            color: #d8dee9;
        }
        
        QAbstractItemView {
            background-color: #3b4252;
            color: #d8dee9;
            selection-background-color: #88c0d0;
            border: 1px solid #434c5e;
        }
    )";
}

QString Theme::getStylesheet(ThemeType theme) {
    switch (theme) {
        case ThemeType::Dark:
            return getDarkStylesheet();
        case ThemeType::Light:
            return getLightStylesheet();
        case ThemeType::SolarizedDark:
            return getSolarizedDarkStylesheet();
        case ThemeType::SolarizedLight:
            return getSolarizedLightStylesheet();
        case ThemeType::Dracula:
            return getDraculaStylesheet();
        case ThemeType::Nord:
            return getNordStylesheet();
        default:
            return getDarkStylesheet();
    }
}

QStringList Theme::getAvailableThemes() {
    return {"Dark", "Light", "Solarized Dark", "Solarized Light", "Dracula", "Nord"};
}
