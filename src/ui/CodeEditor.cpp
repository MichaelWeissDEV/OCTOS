#include "CodeEditor.h"
#include "Theme.h"
#include "../utils/OpcodeReference.h"

#include <QFont>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QMap>
#include <QPainter>
#include <QTextBlock>
#include <QTextEdit>
#include <QToolTip>

namespace {
QColor markerColor(EditorMarker::Severity s) {
    const ThemeTokens& t = Theme::currentTokens();
    switch (s) {
        case EditorMarker::Error: return t.error;
        case EditorMarker::Warning: return t.warning;
        case EditorMarker::Note: return t.info;
    }
    return t.error;
}
}

CodeEditor::CodeEditor(QWidget* parent) : QPlainTextEdit(parent) {
    m_lineNumberArea = new LineNumberArea(this);

    QFont monoFont("Monospace", 11);
    monoFont.setStyleHint(QFont::TypeWriter);
    setFont(monoFont);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * m_indentWidth);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::refreshExtraSelections);

    updateLineNumberAreaWidth(0);
    refreshExtraSelections();
}

int CodeEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    // room for the digits plus a marker column
    return 14 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void CodeEditor::applyTheme() {
    refreshExtraSelections();
    m_lineNumberArea->update();
}

void CodeEditor::goToLine(int line) {
    QTextBlock block = document()->findBlockByNumber(qMax(0, line - 1));
    if (!block.isValid()) return;
    QTextCursor c(block);
    c.select(QTextCursor::LineUnderCursor);
    setTextCursor(c);
    centerCursor();
    setFocus();
}

void CodeEditor::setMarkers(const QVector<EditorMarker>& markers) {
    m_markers = markers;
    refreshExtraSelections();
    m_lineNumberArea->update();
}

QString CodeEditor::markerAtLine(int line, EditorMarker::Severity* sev) const {
    QStringList msgs;
    bool have = false;
    EditorMarker::Severity worst = EditorMarker::Note;
    for (const EditorMarker& m : m_markers) {
        if (m.line != line) continue;
        msgs << m.message;
        if (!have || m.severity < worst) worst = m.severity;  // Error < Warning < Note
        have = true;
    }
    if (sev && have) *sev = worst;
    return msgs.join('\n');
}

void CodeEditor::enableOpcodeTooltips(bool enable) {
    m_opcodeTooltipsEnabled = enable;
    setMouseTracking(enable);
    if (viewport()) {
        viewport()->setMouseTracking(enable);
        if (enable) {
            viewport()->installEventFilter(this);
        } else {
            viewport()->removeEventFilter(this);
        }
    }
}

bool CodeEditor::eventFilter(QObject* watched, QEvent* event) {
    if (watched == viewport() && m_opcodeTooltipsEnabled && event->type() == QEvent::ToolTip) {
        auto* helpEvent = static_cast<QHelpEvent*>(event);
        QTextCursor cursor = cursorForPosition(helpEvent->pos());
        QTextBlock block = cursor.block();
        if (block.isValid()) {
            QString lineText = block.text();
            int column = cursor.positionInBlock();
            QString mnemonic;
            QString arch;
            if (OpcodeReference::extractMnemonicAtColumn(lineText, column, &mnemonic, &arch)) {
                OpcodeInfo info = OpcodeReference::instance().getOpcodeInfo(mnemonic, arch);
                QToolTip::showText(helpEvent->globalPos(), info.toFormattedTooltip(), viewport());
                return true;
            }
        }
        QToolTip::hideText();
        event->ignore();
        return true;
    }
    return QPlainTextEdit::eventFilter(watched, event);
}

bool CodeEditor::event(QEvent* event) {
    if (m_opcodeTooltipsEnabled && event->type() == QEvent::ToolTip) {
        auto* helpEvent = static_cast<QHelpEvent*>(event);
        QTextCursor cursor = cursorForPosition(helpEvent->pos());
        QTextBlock block = cursor.block();
        if (block.isValid()) {
            QString lineText = block.text();
            int column = cursor.positionInBlock();
            QString mnemonic;
            QString arch;
            if (OpcodeReference::extractMnemonicAtColumn(lineText, column, &mnemonic, &arch)) {
                OpcodeInfo info = OpcodeReference::instance().getOpcodeInfo(mnemonic, arch);
                QToolTip::showText(helpEvent->globalPos(), info.toFormattedTooltip(), this);
                return true;
            }
        }
        QToolTip::hideText();
        event->ignore();
        return true;
    }
    return QPlainTextEdit::event(event);
}

void CodeEditor::indentSelection(bool unindent) {
    QTextCursor c = textCursor();
    const QString unit = m_insertSpaces ? QString(m_indentWidth, ' ') : QString("\t");
    if (!c.hasSelection() && !unindent) {
        // Tab at a caret: advance to the next tab stop (spaces) or insert a real tab.
        if (m_insertSpaces) {
            const int col = c.positionInBlock();
            c.insertText(QString(m_indentWidth - (col % m_indentWidth), ' '));
        } else {
            c.insertText("\t");
        }
        return;
    }
    const int startBlock = document()->findBlock(c.selectionStart()).blockNumber();
    int endBlock = document()->findBlock(c.selectionEnd()).blockNumber();
    // A selection that ends at column 0 of a line does not include that line.
    if (c.hasSelection() && document()->findBlock(c.selectionEnd()).position() == c.selectionEnd() && endBlock > startBlock)
        --endBlock;
    c.beginEditBlock();
    for (int b = startBlock; b <= endBlock; ++b) {
        QTextCursor lc(document()->findBlockByNumber(b));
        if (unindent) {
            const QString t = lc.block().text();
            int remove = 0;
            if (t.startsWith('\t')) remove = 1;
            else while (remove < m_indentWidth && remove < t.size() && t[remove] == ' ') ++remove;
            if (remove > 0) { lc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, remove); lc.removeSelectedText(); }
        } else {
            lc.insertText(unit);
        }
    }
    c.endEditBlock();
}

void CodeEditor::insertNewlineWithIndent() {
    QTextCursor c = textCursor();
    const QString line = c.block().text().left(c.positionInBlock());
    QString indent;
    for (QChar ch : line) { if (ch == ' ' || ch == '\t') indent += ch; else break; }
    // After an opening brace/colon, indent one level deeper.
    const QString trimmed = line.trimmed();
    if (trimmed.endsWith('{') || trimmed.endsWith(':')) indent += m_insertSpaces ? QString(m_indentWidth, ' ') : QString("\t");
    c.insertText("\n" + indent);
    setTextCursor(c);
}

void CodeEditor::keyPressEvent(QKeyEvent* event) {
    if (isReadOnly()) {
        QPlainTextEdit::keyPressEvent(event);
        return;
    }
    if (event->key() == Qt::Key_Tab && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
        indentSelection(false);
        return;
    }
    if (event->key() == Qt::Key_Backtab) {
        indentSelection(true);
        return;
    }
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) &&
        !(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))) {
        insertNewlineWithIndent();
        return;
    }

    const QChar ch = event->text().isEmpty() ? QChar() : event->text().at(0);
    const QMap<QChar, QChar> pairs = {{'(', ')'}, {'[', ']'}, {'{', '}'}, {'"', '"'}, {'\'', '\''}};

    if (pairs.contains(ch) && !textCursor().hasSelection()) {
        // Do not auto-pair a quote that directly follows a word character (apostrophes in comments).
        const QTextCursor cur = textCursor();
        const QChar prev = cur.positionInBlock() > 0 ? cur.block().text().at(cur.positionInBlock() - 1) : QChar();
        if ((ch == '"' || ch == '\'') && (prev.isLetterOrNumber() || prev == '\\')) {
            QPlainTextEdit::keyPressEvent(event);
            return;
        }
        QPlainTextEdit::keyPressEvent(event);
        QChar closing = pairs.value(ch);
        insertPlainText(QString(closing));
        QTextCursor c = textCursor();
        c.movePosition(QTextCursor::Left);
        setTextCursor(c);
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::refreshExtraSelections() {
    QList<QTextEdit::ExtraSelection> extra;
    const ThemeTokens& t = Theme::currentTokens();

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(t.currentLine);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extra.append(selection);
    }

    for (const EditorMarker& m : std::as_const(m_markers)) {
        QTextBlock block = document()->findBlockByNumber(m.line - 1);
        if (!block.isValid()) continue;
        QTextEdit::ExtraSelection sel;
        sel.cursor = QTextCursor(block);
        if (m.column > 0 && m.column - 1 < block.length()) {
            sel.cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, m.column - 1);
            sel.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            if (!sel.cursor.hasSelection()) sel.cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
        } else {
            sel.cursor.select(QTextCursor::LineUnderCursor);
        }
        sel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        sel.format.setUnderlineColor(markerColor(m.severity));
        extra.append(sel);
    }
    setExtraSelections(extra);
}

void CodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    const ThemeTokens& t = Theme::currentTokens();
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), t.window);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const int line = blockNumber + 1;
            EditorMarker::Severity sev = EditorMarker::Note;
            if (!markerAtLine(line, &sev).isEmpty()) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(markerColor(sev));
                const int h = fontMetrics().height();
                painter.drawEllipse(QRect(2, top + (h - 8) / 2, 8, 8));
            }
            painter.setPen(t.textMuted);
            painter.drawText(0, top, m_lineNumberArea->width() - 3, fontMetrics().height(),
                             Qt::AlignRight, QString::number(line));
        }

        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
        ++blockNumber;
    }
}

QSize LineNumberArea::sizeHint() const { return QSize(m_codeEditor->lineNumberAreaWidth(), 0); }

void LineNumberArea::paintEvent(QPaintEvent* event) {
    m_codeEditor->lineNumberAreaPaintEvent(event);
}

bool LineNumberArea::event(QEvent* e) {
    // Hovering a marker in the gutter shows the compiler message.
    if (e->type() == QEvent::ToolTip) {
        auto* he = static_cast<QHelpEvent*>(e);
        QTextCursor c = m_codeEditor->cursorForPosition(QPoint(0, he->pos().y()));
        const int line = c.blockNumber() + 1;
        QString text;
        for (const EditorMarker& m : m_codeEditor->markers())
            if (m.line == line) text += (text.isEmpty() ? "" : "\n") + m.message;
        if (!text.isEmpty()) QToolTip::showText(he->globalPos(), text, this);
        else QToolTip::hideText();
        return true;
    }
    return QWidget::event(e);
}
