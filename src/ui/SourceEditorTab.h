#ifndef SOURCEEDITORTAB_H
#define SOURCEEDITORTAB_H

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include "CodeEditor.h"

// Find / replace for one editor (Ctrl+F opens it, Esc closes it).
class FindReplaceBar : public QWidget {
    Q_OBJECT

public:
    explicit FindReplaceBar(CodeEditor* editor, QWidget* parent = nullptr);

    void openBar(bool withReplace = false);
    // Programmatic access (used by tests and by "find next" shortcuts)
    bool findNext(bool backwards = false);
    int replaceAll();
    bool replaceCurrent();
    void setFindText(const QString& t) { m_find->setText(t); }
    void setReplaceText(const QString& t) { m_replace->setText(t); }
    void setCaseSensitive(bool on) { m_case->setChecked(on); }
    QString statusText() const { return m_status->text(); }

private:
    QTextDocument::FindFlags flags(bool backwards) const;
    void updateStatus();

    CodeEditor* m_editor;
    QLineEdit* m_find;
    QLineEdit* m_replace;
    QCheckBox* m_case;
    QLabel* m_status;
    QWidget* m_replaceRow;
};

// One editor tab: the code editor plus its find/replace bar.
class SourceEditorTab : public QWidget {
    Q_OBJECT

public:
    explicit SourceEditorTab(QWidget* parent = nullptr);

    CodeEditor* editor() const { return m_editor; }
    FindReplaceBar* findBar() const { return m_bar; }

private:
    CodeEditor* m_editor;
    FindReplaceBar* m_bar;
};

#endif // SOURCEEDITORTAB_H
