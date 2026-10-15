#include "SourceEditorTab.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QShortcut>
#include <QVBoxLayout>

FindReplaceBar::FindReplaceBar(CodeEditor* editor, QWidget* parent) : QWidget(parent), m_editor(editor) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(6, 4, 6, 4);
    v->setSpacing(4);
    Theme::setRole(this, "toolbar");

    auto* findRow = new QHBoxLayout();
    m_find = new QLineEdit();
    m_find->setPlaceholderText("Find");
    m_find->setClearButtonEnabled(true);
    auto* prev = new QPushButton("▲");
    prev->setToolTip("Previous match (Shift+Enter)");
    prev->setFixedWidth(30);
    auto* next = new QPushButton("▼");
    next->setToolTip("Next match (Enter)");
    next->setFixedWidth(30);
    m_case = new QCheckBox("Match case");
    m_status = new QLabel();
    Theme::setRole(m_status, "muted");
    auto* close = new QPushButton("✕");
    close->setFixedWidth(30);
    Theme::setRole(close, "subtle");
    close->setToolTip("Close (Esc)");
    findRow->addWidget(m_find, 1);
    findRow->addWidget(prev);
    findRow->addWidget(next);
    findRow->addWidget(m_case);
    findRow->addWidget(m_status);
    findRow->addWidget(close);
    v->addLayout(findRow);

    m_replaceRow = new QWidget();
    auto* repRow = new QHBoxLayout(m_replaceRow);
    repRow->setContentsMargins(0, 0, 0, 0);
    m_replace = new QLineEdit();
    m_replace->setPlaceholderText("Replace with");
    auto* rep = new QPushButton("Replace");
    auto* repAll = new QPushButton("Replace all");
    repRow->addWidget(m_replace, 1);
    repRow->addWidget(rep);
    repRow->addWidget(repAll);
    v->addWidget(m_replaceRow);
    m_replaceRow->setVisible(false);
    setVisible(false);

    connect(next, &QPushButton::clicked, this, [this]() { findNext(false); });
    connect(prev, &QPushButton::clicked, this, [this]() { findNext(true); });
    connect(m_find, &QLineEdit::returnPressed, this, [this]() { findNext(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier); });
    connect(m_find, &QLineEdit::textChanged, this, [this]() { updateStatus(); });
    connect(m_case, &QCheckBox::toggled, this, [this]() { updateStatus(); });
    connect(close, &QPushButton::clicked, this, [this]() { hide(); m_editor->setFocus(); });
    connect(rep, &QPushButton::clicked, this, [this]() { replaceCurrent(); });
    connect(repAll, &QPushButton::clicked, this, [this]() { replaceAll(); });

    // Shortcuts live on the editor so they work while it has focus.
    auto* find = new QShortcut(QKeySequence::Find, m_editor);
    find->setContext(Qt::WidgetWithChildrenShortcut);
    connect(find, &QShortcut::activated, this, [this]() { openBar(false); });
    auto* replace = new QShortcut(QKeySequence::Replace, m_editor);
    replace->setContext(Qt::WidgetWithChildrenShortcut);
    connect(replace, &QShortcut::activated, this, [this]() { openBar(true); });
    auto* nextSc = new QShortcut(QKeySequence::FindNext, m_editor);
    nextSc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(nextSc, &QShortcut::activated, this, [this]() { findNext(false); });
    auto* prevSc = new QShortcut(QKeySequence::FindPrevious, m_editor);
    prevSc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(prevSc, &QShortcut::activated, this, [this]() { findNext(true); });
    auto* esc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    esc->setContext(Qt::WidgetWithChildrenShortcut);
    connect(esc, &QShortcut::activated, this, [this]() { hide(); m_editor->setFocus(); });
}

QTextDocument::FindFlags FindReplaceBar::flags(bool backwards) const {
    QTextDocument::FindFlags f;
    if (backwards) f |= QTextDocument::FindBackward;
    if (m_case->isChecked()) f |= QTextDocument::FindCaseSensitively;
    return f;
}

void FindReplaceBar::openBar(bool withReplace) {
    setVisible(true);
    m_replaceRow->setVisible(withReplace && !m_editor->isReadOnly());
    const QString sel = m_editor->textCursor().selectedText();
    if (!sel.isEmpty() && !sel.contains(QChar::ParagraphSeparator)) m_find->setText(sel);
    m_find->setFocus();
    m_find->selectAll();
    updateStatus();
}

bool FindReplaceBar::findNext(bool backwards) {
    const QString needle = m_find->text();
    if (needle.isEmpty()) return false;
    bool found = m_editor->find(needle, flags(backwards));
    if (!found) {   // wrap around
        QTextCursor c = m_editor->textCursor();
        c.movePosition(backwards ? QTextCursor::End : QTextCursor::Start);
        m_editor->setTextCursor(c);
        found = m_editor->find(needle, flags(backwards));
    }
    m_status->setText(found ? QString() : "No match");
    return found;
}

bool FindReplaceBar::replaceCurrent() {
    if (m_editor->isReadOnly() || m_find->text().isEmpty()) return false;
    QTextCursor c = m_editor->textCursor();
    const bool cs = m_case->isChecked();
    if (c.hasSelection() && c.selectedText().compare(m_find->text(), cs ? Qt::CaseSensitive : Qt::CaseInsensitive) == 0) {
        c.insertText(m_replace->text());
        m_editor->setTextCursor(c);
    }
    return findNext(false);
}

int FindReplaceBar::replaceAll() {
    if (m_editor->isReadOnly() || m_find->text().isEmpty()) return 0;
    int n = 0;
    QTextCursor c(m_editor->document());
    c.beginEditBlock();
    QTextCursor pos(m_editor->document());
    pos.movePosition(QTextCursor::Start);
    for (;;) {
        pos = m_editor->document()->find(m_find->text(), pos, flags(false));
        if (pos.isNull()) break;
        pos.insertText(m_replace->text());
        ++n;
    }
    c.endEditBlock();
    m_status->setText(QString("%1 replaced").arg(n));
    return n;
}

void FindReplaceBar::updateStatus() {
    const QString needle = m_find->text();
    if (needle.isEmpty()) { m_status->clear(); return; }
    int count = 0;
    QTextCursor pos(m_editor->document());
    while (count < 100000) {
        pos = m_editor->document()->find(needle, pos, flags(false));
        if (pos.isNull()) break;
        ++count;
    }
    m_status->setText(QString("%1 match%2").arg(count).arg(count == 1 ? "" : "es"));
}

SourceEditorTab::SourceEditorTab(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_editor = new CodeEditor();
    m_bar = new FindReplaceBar(m_editor);
    layout->addWidget(m_editor, 1);
    layout->addWidget(m_bar);
}
