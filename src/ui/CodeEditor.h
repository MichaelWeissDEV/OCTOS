#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QColor>
#include <QPlainTextEdit>
#include <QVector>
#include <QWidget>

class LineNumberArea;

// A marker on a source line (compiler error/warning). Shown in the gutter and as an underline.
struct EditorMarker {
    enum Severity { Error, Warning, Note };
    int line{0};      // 1-based
    int column{0};    // 1-based, 0 = whole line
    Severity severity{Error};
    QString message;
};

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);
    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth();
    void enableOpcodeTooltips(bool enable = true);

    // Replaces all markers (pass an empty list to clear).
    void setMarkers(const QVector<EditorMarker>& markers);
    const QVector<EditorMarker>& markers() const { return m_markers; }

    // Indentation settings
    void setIndentWidth(int spaces) { m_indentWidth = qBound(1, spaces, 16); }
    int indentWidth() const { return m_indentWidth; }
    void setInsertSpaces(bool on) { m_insertSpaces = on; }

    // Re-reads colors from the current theme (line numbers, current line, markers).
    void applyTheme();

    // Selects the line (1-based) and scrolls to it.
    void goToLine(int line);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int);
    void refreshExtraSelections();
    void updateLineNumberArea(const QRect&, int);

private:
    void indentSelection(bool unindent);
    void insertNewlineWithIndent();
    QString markerAtLine(int line, EditorMarker::Severity* sev) const;

    LineNumberArea* m_lineNumberArea;
    bool m_opcodeTooltipsEnabled{false};
    QVector<EditorMarker> m_markers;
    int m_indentWidth{4};
    bool m_insertSpaces{true};
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor* editor) : QWidget(editor), m_codeEditor(editor) {}

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    bool event(QEvent* event) override;

private:
    CodeEditor* m_codeEditor;
};

#endif // CODEEDITOR_H
