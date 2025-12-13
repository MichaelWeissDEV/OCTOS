# 1 "./src/CompilerDriver.h"
#ifndef COMPILERDRIVER_H
#define COMPILERDRIVER_H

#include <QMap>
#include <QString>
#include <QStringList>

enum class SyntaxFlavor { Intel, ATT };
enum class Language { Cpp, C, Rust, CSharp, Java, Python, Ada };

struct CompileResult {
    bool success;
    QString output;
    QMap<int, int> lineMapping;
};

class CompilerDriver {
   public:
    CompilerDriver();

    CompileResult compile(const QString& source, const QString& compiler, const QString& flags,
                          Language lang = Language::Cpp, SyntaxFlavor flavor = SyntaxFlavor::Intel);

    QString getCompilerForLanguage(Language lang);
    QStringList getAvailableCompilers(const QString& baseName);

   private:
    QString cleanAssembly(const QString& rawAsm);
    QString convertAssemblySyntax(const QString& asm_, SyntaxFlavor flavor);
    QMap<int, int> buildLineMapping(const QString& rawAsm);
};

#endif
