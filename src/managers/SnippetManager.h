# 1 "./src/managers/SnippetManager.h"
#ifndef SNIPPETMANAGER_H
#define SNIPPETMANAGER_H

#include <QMap>
#include <QString>
#include <QStringList>

struct CodeSnippet {
    QString name;
    QString description;
    QString code;
    QString language;
};

class SnippetManager {
   public:
    SnippetManager();

    void addSnippet(const CodeSnippet &snippet);
    void removeSnippet(const QString &name);
    CodeSnippet getSnippet(const QString &name) const;

    QList<CodeSnippet> getSnippetsByLanguage(const QString &language) const;
    QStringList getAllSnippetNames() const;

    void loadDefaultSnippets();

   private:
    QMap<QString, CodeSnippet> m_snippets;
};

#endif
