// Theme tokens: completeness, readable contrast in every theme, roles reach widgets.
#include "../TestSupport.h"
#include <QTest>
#include <QApplication>
#include <QPushButton>
#include <QLabel>
#include <QSignalSpy>
#include <cmath>
#include "ui/Theme.h"
#include "ui/CodeEditor.h"
#include "highlighters/PythonHighlighter.h"
#include "highlighters/RustHighlighter.h"

namespace {
double luminance(const QColor& c) {
    auto lin = [](double v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * lin(c.redF()) + 0.7152 * lin(c.greenF()) + 0.0722 * lin(c.blueF());
}
double contrast(const QColor& a, const QColor& b) {
    const double la = luminance(a), lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}
}

class TestTheme : public QObject {
    Q_OBJECT
private slots:
    void testEveryThemeHasCompleteTokens();
    void testBodyTextContrast();
    void testControlAndStateContrast();
    void testSyntaxColorsAreReadable();
    void testStylesheetContainsRolesAndFocus();
    void testRoleReachesWidget();
    void testGenerationDrivesHighlighterRecolor();
    void testEditorMarkersAndIndent();
};

void TestTheme::testEveryThemeHasCompleteTokens() {
    for (int i = 0; i < Theme::getAvailableThemes().size(); ++i) {
        const ThemeTokens t = Theme::tokens(static_cast<ThemeType>(i));
        const QString n = Theme::getAvailableThemes()[i];
        for (const QColor& c : {t.window, t.base, t.panelHeader, t.border, t.text, t.textMuted, t.accent, t.accentText,
                                t.success, t.warning, t.error, t.info, t.syntaxKeyword, t.syntaxType, t.syntaxString,
                                t.syntaxNumber, t.syntaxComment, t.syntaxFunction, t.syntaxPreprocessor, t.syntaxRegister, t.syntaxLabel})
            QVERIFY2(c.isValid(), qPrintable(n));
        QVERIFY2(t.diffAdded.alpha() < 255 && t.diffRemoved.alpha() < 255, qPrintable(n));  // tints, not opaque fills
        QVERIFY(t.focusWidth >= 2);
    }
}

void TestTheme::testBodyTextContrast() {
    for (int i = 0; i < Theme::getAvailableThemes().size(); ++i) {
        const ThemeTokens t = Theme::tokens(static_cast<ThemeType>(i));
        const QString n = Theme::getAvailableThemes()[i];
        QVERIFY2(contrast(t.text, t.window) >= 4.5, qPrintable(QString("%1 text/window %2").arg(n).arg(contrast(t.text, t.window))));
        QVERIFY2(contrast(t.text, t.base) >= 4.5, qPrintable(QString("%1 text/base %2").arg(n).arg(contrast(t.text, t.base))));
        QVERIFY2(contrast(t.textMuted, t.window) >= 3.0, qPrintable(QString("%1 muted/window %2").arg(n).arg(contrast(t.textMuted, t.window))));
    }
}

void TestTheme::testControlAndStateContrast() {
    for (int i = 0; i < Theme::getAvailableThemes().size(); ++i) {
        const ThemeTokens t = Theme::tokens(static_cast<ThemeType>(i));
        const QString n = Theme::getAvailableThemes()[i];
        QVERIFY2(contrast(t.accentText, t.accent) >= 3.0, qPrintable(QString("%1 accentText/accent %2").arg(n).arg(contrast(t.accentText, t.accent))));
        QVERIFY2(contrast(t.accent, t.window) >= 3.0, qPrintable(QString("%1 focus ring (accent/window) %2").arg(n).arg(contrast(t.accent, t.window))));
        for (const QColor& c : {t.success, t.warning, t.error})
            QVERIFY2(contrast(c, t.window) >= 3.0, qPrintable(QString("%1 state color %2 on window %3").arg(n, c.name()).arg(contrast(c, t.window))));
    }
}

void TestTheme::testSyntaxColorsAreReadable() {
    for (int i = 0; i < Theme::getAvailableThemes().size(); ++i) {
        const ThemeTokens t = Theme::tokens(static_cast<ThemeType>(i));
        const QString n = Theme::getAvailableThemes()[i];
        struct { const char* name; QColor c; } cols[] = {{"keyword", t.syntaxKeyword}, {"type", t.syntaxType}, {"string", t.syntaxString},
            {"number", t.syntaxNumber}, {"function", t.syntaxFunction}, {"preprocessor", t.syntaxPreprocessor}, {"register", t.syntaxRegister},
            {"label", t.syntaxLabel}, {"comment", t.syntaxComment}};
        for (const auto& col : cols) {
            const double min = QString(col.name) == "comment" ? 2.5 : 3.0;   // comments are intentionally quieter
            QVERIFY2(contrast(col.c, t.window) >= min,
                     qPrintable(QString("%1 syntax %2 (%3) on window: %4").arg(n, col.name, col.c.name()).arg(contrast(col.c, t.window))));
        }
    }
}

void TestTheme::testStylesheetContainsRolesAndFocus() {
    for (int i = 0; i < Theme::getAvailableThemes().size(); ++i) {
        const QString css = Theme::getStylesheet(static_cast<ThemeType>(i));
        for (const char* role : {"accent", "success", "danger", "subtle", "muted", "ok", "warning", "error", "toolbar", "noticeWarning", "noticeError", "noticeOk"})
            QVERIFY2(css.contains(QString("[role=\"%1\"]").arg(role)), role);
        QVERIFY(css.contains(":focus"));
        QVERIFY(css.contains(Theme::tokens(static_cast<ThemeType>(i)).accent.name()));
    }
}

void TestTheme::testRoleReachesWidget() {
    QLabel label("x");
    Theme::setRole(&label, "error");
    QCOMPARE(label.property("role").toString(), QString("error"));
    Theme::setRole(nullptr, "error");   // must not crash
}

void TestTheme::testGenerationDrivesHighlighterRecolor() {
    Theme::setCurrent(ThemeType::Dark);
    QTextDocument doc("def f():\n    return 1\n");
    PythonHighlighter hl(&doc);
    const int g0 = Theme::generation();
    Theme::setCurrent(ThemeType::Light);
    QVERIFY(Theme::generation() > g0);
    hl.applyTheme();
    const QTextCharFormat f = doc.firstBlock().layout()->formats().isEmpty() ? QTextCharFormat() : doc.firstBlock().layout()->formats().first().format;
    // The first token ("def") is a keyword: it must now carry the LIGHT theme's keyword color.
    QVERIFY(!doc.firstBlock().layout()->formats().isEmpty());
    QCOMPARE(f.foreground().color(), Theme::tokens(ThemeType::Light).syntaxKeyword);
    Theme::setCurrent(ThemeType::Dark);
}

void TestTheme::testEditorMarkersAndIndent() {
    CodeEditor ed;
    ed.setPlainText("int main() {\nreturn x;\n}\n");
    ed.setMarkers({{2, 8, EditorMarker::Error, "'x' was not declared"}});
    QCOMPARE(ed.markers().size(), 1);
    QVERIFY(ed.extraSelections().size() >= 2);          // current line + marker underline
    ed.setMarkers({});
    QCOMPARE(ed.markers().size(), 0);

    // Tab inserts spaces to the next stop; Shift+Tab removes one level; Enter keeps indentation.
    CodeEditor e2;
    e2.show();
    e2.setPlainText("");
    QTest::keyClick(&e2, Qt::Key_Tab);
    QCOMPARE(e2.toPlainText(), QString("    "));
    QTest::keyClick(&e2, Qt::Key_Return);
    QCOMPARE(e2.toPlainText(), QString("    \n    "));
    QTest::keyClick(&e2, Qt::Key_Backtab);
    QCOMPARE(e2.toPlainText(), QString("    \n"));
    e2.setPlainText("a\nb\nc");
    QTextCursor c = e2.textCursor();
    c.setPosition(0); c.setPosition(e2.toPlainText().size(), QTextCursor::KeepAnchor);
    e2.setTextCursor(c);
    QTest::keyClick(&e2, Qt::Key_Tab);
    QCOMPARE(e2.toPlainText(), QString("    a\n    b\n    c"));
    QTest::keyClick(&e2, Qt::Key_Backtab);
    QCOMPARE(e2.toPlainText(), QString("a\nb\nc"));
    e2.setPlainText("");
    e2.setPlainText("if (x) {");
    QTextCursor end = e2.textCursor(); end.movePosition(QTextCursor::End); e2.setTextCursor(end);
    QTest::keyClick(&e2, Qt::Key_Return);
    QCOMPARE(e2.toPlainText(), QString("if (x) {\n    "));  // opening brace indents the next line
}

OCTOS_TEST_MAIN(TestTheme)
#include "test_theme.moc"
