#include "Theme.h"

#include <QStringList>
#include <QStyle>
#include <QWidget>

namespace {

ThemeType g_currentTheme = ThemeType::Dark;
int g_generation = 0;
ThemeTokens g_currentTokens;
bool g_tokensInit = false;

QColor c(const char* hex) { return QColor(hex); }
QColor tint(int r, int g, int b, int a) { return QColor(r, g, b, a); }

ThemeTokens makeTokens(ThemeType type) {
    ThemeTokens t;
    // Shared tints work on dark and light surfaces alike.
    t.diffAdded = tint(46, 160, 67, 70);
    t.diffRemoved = tint(248, 81, 73, 70);
    t.searchMatch = tint(210, 153, 34, 110);
    t.sourceHighlight = tint(14, 99, 156, 80);
    switch (type) {
        case ThemeType::Dark:
        default:
            t.window = c("#1e1e1e"); t.base = c("#252526"); t.panelHeader = c("#2d2d30");
            t.border = c("#3e3e42"); t.borderStrong = c("#555555");
            t.text = c("#e0e0e0"); t.textMuted = c("#9a9a9a");
            t.accent = c("#007acc"); t.accentHover = c("#1177bb"); t.accentText = c("#ffffff"); t.selection = c("#094771");
            t.success = c("#4ec9b0"); t.warning = c("#e5c07b"); t.error = c("#f48771"); t.info = c("#4fc1ff");
            t.currentLine = tint(255, 255, 255, 16);
            t.syntaxKeyword = c("#569CD6"); t.syntaxType = c("#4EC9B0"); t.syntaxString = c("#CE9178");
            t.syntaxNumber = c("#B5CEA8"); t.syntaxComment = c("#6A9955"); t.syntaxFunction = c("#DCDCAA");
            t.syntaxPreprocessor = c("#9CDCFE"); t.syntaxRegister = c("#C586C0"); t.syntaxLabel = c("#6A9955");
            break;
        case ThemeType::Light:
            t.window = c("#ffffff"); t.base = c("#f5f5f5"); t.panelHeader = c("#ebebeb");
            t.border = c("#d0d0d0"); t.borderStrong = c("#a0a0a0");
            t.text = c("#1f1f1f"); t.textMuted = c("#606060");
            t.accent = c("#0066cc"); t.accentHover = c("#0052a3"); t.accentText = c("#ffffff"); t.selection = c("#cce4f7");
            t.success = c("#1a7f37"); t.warning = c("#9a6700"); t.error = c("#c62828"); t.info = c("#0969da");
            t.currentLine = tint(0, 0, 0, 14);
            t.syntaxKeyword = c("#0000ff"); t.syntaxType = c("#267f99"); t.syntaxString = c("#a31515");
            t.syntaxNumber = c("#098658"); t.syntaxComment = c("#008000"); t.syntaxFunction = c("#795e26");
            t.syntaxPreprocessor = c("#af00db"); t.syntaxRegister = c("#8f08c4"); t.syntaxLabel = c("#008000");
            break;
        case ThemeType::SolarizedDark:
            t.window = c("#002b36"); t.base = c("#073642"); t.panelHeader = c("#073642");
            t.border = c("#586e75"); t.borderStrong = c("#657b83");
            t.text = c("#93a1a1"); t.textMuted = c("#657b83");
            t.accent = c("#268bd2"); t.accentHover = c("#2aa198"); t.accentText = c("#002b36"); t.selection = c("#0a4a5a");
            t.success = c("#859900"); t.warning = c("#b58900"); t.error = c("#dc322f"); t.info = c("#2aa198");
            t.currentLine = tint(255, 255, 255, 14);
            t.syntaxKeyword = c("#859900"); t.syntaxType = c("#b58900"); t.syntaxString = c("#2aa198");
            t.syntaxNumber = c("#d33682"); t.syntaxComment = c("#586e75"); t.syntaxFunction = c("#268bd2");
            t.syntaxPreprocessor = c("#cb4b16"); t.syntaxRegister = c("#6c71c4"); t.syntaxLabel = c("#268bd2");
            break;
        case ThemeType::SolarizedLight:
            t.window = c("#fdf6e3"); t.base = c("#eee8d5"); t.panelHeader = c("#eee8d5");
            t.border = c("#d6d0c8"); t.borderStrong = c("#93a1a1");
            t.text = c("#3f565d"); t.textMuted = c("#657b83");
            t.accent = c("#1f6fa8"); t.accentHover = c("#1a8f87"); t.accentText = c("#fdf6e3"); t.selection = c("#d8e8ee");
            t.success = c("#6b7c00"); t.warning = c("#8a6700"); t.error = c("#dc322f"); t.info = c("#2aa198");
            t.currentLine = tint(0, 0, 0, 12);
            t.syntaxKeyword = c("#6b7c00"); t.syntaxType = c("#8a6700"); t.syntaxString = c("#1a7f78");
            t.syntaxNumber = c("#c02a73"); t.syntaxComment = c("#7f9195"); t.syntaxFunction = c("#1f6fa8");
            t.syntaxPreprocessor = c("#b3440f"); t.syntaxRegister = c("#5f64b8"); t.syntaxLabel = c("#1f6fa8");
            break;
        case ThemeType::Dracula:
            t.window = c("#282a36"); t.base = c("#44475a"); t.panelHeader = c("#343746");
            t.border = c("#6272a4"); t.borderStrong = c("#7b8bbd");
            t.text = c("#f8f8f2"); t.textMuted = c("#a4adc9");
            t.accent = c("#bd93f9"); t.accentHover = c("#8be9fd"); t.accentText = c("#282a36"); t.selection = c("#5b4a85");
            t.success = c("#50fa7b"); t.warning = c("#f1fa8c"); t.error = c("#ff5555"); t.info = c("#8be9fd");
            t.currentLine = tint(255, 255, 255, 14);
            t.syntaxKeyword = c("#ff79c6"); t.syntaxType = c("#8be9fd"); t.syntaxString = c("#f1fa8c");
            t.syntaxNumber = c("#bd93f9"); t.syntaxComment = c("#6272a4"); t.syntaxFunction = c("#50fa7b");
            t.syntaxPreprocessor = c("#ffb86c"); t.syntaxRegister = c("#bd93f9"); t.syntaxLabel = c("#50fa7b");
            break;
        case ThemeType::Nord:
            t.window = c("#2e3440"); t.base = c("#3b4252"); t.panelHeader = c("#343b49");
            t.border = c("#434c5e"); t.borderStrong = c("#4c566a");
            t.text = c("#d8dee9"); t.textMuted = c("#9aa5b8");
            t.accent = c("#88c0d0"); t.accentHover = c("#81a1c1"); t.accentText = c("#2e3440"); t.selection = c("#4c6a78");
            t.success = c("#a3be8c"); t.warning = c("#ebcb8b"); t.error = c("#bf616a"); t.info = c("#88c0d0");
            t.currentLine = tint(255, 255, 255, 14);
            t.syntaxKeyword = c("#81a1c1"); t.syntaxType = c("#8fbcbb"); t.syntaxString = c("#a3be8c");
            t.syntaxNumber = c("#b48ead"); t.syntaxComment = c("#6d7b96"); t.syntaxFunction = c("#88c0d0");
            t.syntaxPreprocessor = c("#d08770"); t.syntaxRegister = c("#b48ead"); t.syntaxLabel = c("#ebcb8b");
            break;
    }
    return t;
}

QString rgba(const QColor& col) {
    return QString("rgba(%1,%2,%3,%4)").arg(col.red()).arg(col.green()).arg(col.blue()).arg(col.alpha());
}

}

static QString baseStylesheet(const ThemeTokens& t) {
    const QString focusBorder = QString("%1px solid %2").arg(t.borderWidth).arg(t.accent.name());
    return QString(R"(
        QWidget { background-color: %1; color: %2; }
        QLineEdit, QPlainTextEdit, QTextEdit, QComboBox, QSpinBox {
            background-color: %3; color: %2; border: %4px solid %5; padding: 4px; border-radius: %6px;
            selection-background-color: %7; selection-color: %2;
        }
        QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus { border: %8; }
        QPushButton { background-color: %9; color: %10; border: none; padding: 6px 12px; border-radius: %6px; font-weight: bold; }
        QPushButton:hover { background-color: %11; }
        QPushButton:pressed { background-color: %12; }
        QLabel { color: %2; background-color: transparent; }
        QSplitter::handle { background-color: %5; }
        QSplitter::handle:hover { background-color: %9; }
        QComboBox::drop-down { border: none; width: 20px; }
        QComboBox::down-arrow { image: none; width: 8px; height: 8px; }
        QAbstractItemView { background-color: %3; color: %2; selection-background-color: %7; selection-color: %2; border: %4px solid %5; }
        QMenuBar, QMenu { background-color: %13; color: %2; }
        QMenu::item:selected, QMenuBar::item:selected { background-color: %7; }
        QToolTip { background-color: %13; color: %2; border: %4px solid %5; }
        QProgressBar { border: %4px solid %5; border-radius: %6px; background: %3; }
        QProgressBar::chunk { background-color: %9; }
        QScrollBar:vertical { background: %3; width: 12px; }
        QScrollBar::handle:vertical { background: %5; min-height: 24px; border-radius: 4px; }
        QScrollBar:horizontal { background: %3; height: 12px; }
        QScrollBar::handle:horizontal { background: %5; min-width: 24px; border-radius: 4px; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
    )")
        .arg(t.window.name(), t.text.name(), t.base.name())
        .arg(t.borderWidth).arg(t.border.name()).arg(t.radius)
        .arg(t.selection.name(), focusBorder, t.accent.name(), t.accentText.name(),
             t.accentHover.name(), t.accentHover.darker(115).name(), t.panelHeader.name());
}

QString Theme::getDarkStylesheet() { return baseStylesheet(makeTokens(ThemeType::Dark)); }
QString Theme::getLightStylesheet() { return baseStylesheet(makeTokens(ThemeType::Light)); }
QString Theme::getSolarizedDarkStylesheet() { return baseStylesheet(makeTokens(ThemeType::SolarizedDark)); }
QString Theme::getSolarizedLightStylesheet() { return baseStylesheet(makeTokens(ThemeType::SolarizedLight)); }
QString Theme::getDraculaStylesheet() { return baseStylesheet(makeTokens(ThemeType::Dracula)); }
QString Theme::getNordStylesheet() { return baseStylesheet(makeTokens(ThemeType::Nord)); }

ThemeTokens Theme::tokens(ThemeType theme) { return makeTokens(theme); }

ThemeType Theme::currentType() { return g_currentTheme; }

const ThemeTokens& Theme::currentTokens() {
    if (!g_tokensInit) { g_currentTokens = makeTokens(g_currentTheme); g_tokensInit = true; }
    return g_currentTokens;
}

void Theme::setCurrent(ThemeType theme) {
    g_currentTheme = theme;
    g_currentTokens = makeTokens(theme);
    g_tokensInit = true;
    ++g_generation;
}

int Theme::generation() { return g_generation; }

QColor Theme::syntaxColor(SyntaxRole role) {
    const ThemeTokens& t = currentTokens();
    switch (role) {
        case SyntaxRole::Keyword: return t.syntaxKeyword;
        case SyntaxRole::Type: return t.syntaxType;
        case SyntaxRole::String: return t.syntaxString;
        case SyntaxRole::Number: return t.syntaxNumber;
        case SyntaxRole::Comment: return t.syntaxComment;
        case SyntaxRole::Function: return t.syntaxFunction;
        case SyntaxRole::Preprocessor: return t.syntaxPreprocessor;
        case SyntaxRole::Register: return t.syntaxRegister;
        case SyntaxRole::Label: return t.syntaxLabel;
    }
    return t.text;
}

void Theme::setRole(QWidget* w, const QString& role) {
    if (!w) return;
    w->setProperty("role", role);
    // Dynamic property changes only take effect after the style re-evaluates the widget.
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

QString Theme::roleStylesheet(const ThemeTokens& t) {
    const QString focus = QString("%1px solid %2").arg(t.focusWidth).arg(t.accent.name());
    QString css;
    auto add = [&css](const QString& rule) { css += rule + "\n"; };

    // Semantic label colors
    add(QString("QLabel[role=\"muted\"] { color: %1; }").arg(t.textMuted.name()));
    add(QString("QLabel[role=\"ok\"] { color: %1; }").arg(t.success.name()));
    add(QString("QLabel[role=\"warning\"] { color: %1; font-weight: bold; }").arg(t.warning.name()));
    add(QString("QLabel[role=\"error\"] { color: %1; font-weight: bold; }").arg(t.error.name()));
    add(QString("QLabel[role=\"title\"] { color: %1; font-weight: bold; }").arg(t.info.name()));
    add(QString("QLabel[role=\"noticeWarning\"] { background-color: %1; border-radius: %2px; padding: 4px; }").arg(rgba(QColor(t.warning.red(), t.warning.green(), t.warning.blue(), 70))).arg(t.radius));
    add(QString("QLabel[role=\"noticeOk\"] { background-color: %1; border-radius: %2px; padding: 8px 12px; font-weight: bold; }").arg(rgba(QColor(t.success.red(), t.success.green(), t.success.blue(), 60))).arg(t.radius));
    add(QString("QLabel[role=\"noticeError\"] { background-color: %1; border-radius: %2px; padding: 4px; }").arg(rgba(QColor(t.error.red(), t.error.green(), t.error.blue(), 70))).arg(t.radius));

    // Surfaces
    add(QString("QWidget[role=\"toolbar\"] { background-color: %1; border-bottom: %2px solid %3; }").arg(t.panelHeader.name()).arg(t.borderWidth).arg(t.border.name()));
    add(QString("QLabel[role=\"toolbarLabel\"] { background-color: transparent; font-weight: bold; }"));

    // Buttons
    add(QString("QPushButton[role=\"accent\"] { background-color: %1; color: %2; } QPushButton[role=\"accent\"]:hover { background-color: %3; }").arg(t.accent.name(), t.accentText.name(), t.accentHover.name()));
    add(QString("QPushButton[role=\"success\"] { background-color: %1; color: %2; } QPushButton[role=\"success\"]:disabled { background-color: %3; color: %4; }").arg(t.success.name(), t.window.name(), t.border.name(), t.textMuted.name()));
    add(QString("QPushButton[role=\"danger\"] { background-color: %1; color: %2; }").arg(t.error.name(), t.window.name()));
    add(QString("QPushButton[role=\"subtle\"] { background-color: %1; color: %2; border: %3px solid %4; } QPushButton[role=\"subtle\"]:hover { background-color: %5; }")
            .arg(t.panelHeader.name(), t.text.name()).arg(t.borderWidth).arg(t.borderStrong.name(), t.border.name()));
    add(QString("QPushButton:disabled { background-color: %1; color: %2; }").arg(t.border.name(), t.textMuted.name()));

    // Keyboard focus must always be visible
    add(QString("QPushButton:focus, QComboBox:focus, QLineEdit:focus, QTableView:focus, QTreeView:focus, QTabBar::tab:focus { outline: none; border: %1; }").arg(focus));

    // Editors and states
    add(QString("QPlainTextEdit[state=\"error\"] { color: %1; }").arg(t.error.name()));
    add(QString("QTableView, QTreeView { gridline-color: %1; alternate-background-color: %2; }").arg(t.border.name(), t.panelHeader.name()));
    add(QString("QTableView::item:selected, QTreeView::item:selected { background-color: %1; color: %2; }").arg(t.selection.name(), t.text.name()));
    add(QString("QHeaderView::section { background-color: %1; color: %2; padding: 4px; border: %3px solid %4; font-weight: bold; }").arg(t.panelHeader.name(), t.text.name()).arg(t.borderWidth).arg(t.border.name()));
    add(QString("QTabBar::tab { background: %1; color: %2; padding: 4px 10px; border: %3px solid %4; } QTabBar::tab:selected { background: %5; border-bottom-color: %5; }").arg(t.panelHeader.name(), t.textMuted.name()).arg(t.borderWidth).arg(t.border.name(), t.window.name()));
    add(QString("QDockWidget::title { background: %1; padding: 4px 6px; }").arg(t.panelHeader.name()));
    return css;
}

QString Theme::getStylesheet(ThemeType theme) {
    QString base;
    switch (theme) {
        case ThemeType::Dark: base = getDarkStylesheet(); break;
        case ThemeType::Light: base = getLightStylesheet(); break;
        case ThemeType::SolarizedDark: base = getSolarizedDarkStylesheet(); break;
        case ThemeType::SolarizedLight: base = getSolarizedLightStylesheet(); break;
        case ThemeType::Dracula: base = getDraculaStylesheet(); break;
        case ThemeType::Nord: base = getNordStylesheet(); break;
        default: base = getDarkStylesheet(); break;
    }
    return base + "\n" + roleStylesheet(makeTokens(theme));
}

QStringList Theme::getAvailableThemes() {
    return {"Dark", "Light", "Solarized Dark", "Solarized Light", "Dracula", "Nord"};
}
