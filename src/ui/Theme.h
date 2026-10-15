#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QString>
#include <QStringList>

class QWidget;

enum class ThemeType { Dark, Light, SolarizedDark, SolarizedLight, Dracula, Nord };

enum class SyntaxRole { Keyword, Type, String, Number, Comment, Function, Preprocessor, Register, Label };

// Central design values. Widgets never hard-code colors: they ask for a role (see setRole) or
// read a token, so every theme - including the light ones - renders correctly.
struct ThemeTokens {
    // Surfaces
    QColor window, base, panelHeader, border, borderStrong;
    // Text
    QColor text, textMuted;
    // Accent / state
    QColor accent, accentHover, accentText, selection;
    QColor success, warning, error, info;
    // Translucent highlight tints (valid on light and dark backgrounds)
    QColor diffAdded, diffRemoved, searchMatch, sourceHighlight, currentLine;
    // Syntax
    QColor syntaxKeyword, syntaxType, syntaxString, syntaxNumber, syntaxComment, syntaxFunction,
           syntaxPreprocessor, syntaxRegister, syntaxLabel;
    // Metrics
    int spacing{8};
    int radius{3};
    int borderWidth{1};
    int focusWidth{2};          // focus indicator thickness
    int fontPointSize{0};       // 0 = application default (scales with HiDPI)
    QString monoFamily{"Monospace"};
};

class Theme {
   public:
    static QString getDarkStylesheet();
    static QString getLightStylesheet();
    static QString getSolarizedDarkStylesheet();
    static QString getSolarizedLightStylesheet();
    static QString getDraculaStylesheet();
    static QString getNordStylesheet();

    // Complete stylesheet for a theme: base rules + token-driven role rules.
    static QString getStylesheet(ThemeType theme);
    static QStringList getAvailableThemes();

    static ThemeTokens tokens(ThemeType theme);
    static ThemeType currentType();
    static const ThemeTokens& currentTokens();
    // Switching bumps generation(); syntax highlighters compare it to know when to recolor.
    static void setCurrent(ThemeType theme);
    static int generation();

    static QColor syntaxColor(SyntaxRole role);

    // Marks a widget with a semantic role ("accent", "success", "danger", "subtle", "muted",
    // "ok", "warning", "error", "toolbar") that the stylesheet colors, and re-polishes it.
    static void setRole(QWidget* w, const QString& role);

    // Token-driven rules shared by every theme (roles, focus indicators, tables).
    static QString roleStylesheet(const ThemeTokens& t);
};

#endif
