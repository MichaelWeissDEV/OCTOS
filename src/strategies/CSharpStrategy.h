# 1 "./src/strategies/CSharpStrategy.h"
#ifndef CSHARPSTRATEGY_H
#define CSHARPSTRATEGY_H

#include "ILanguageStrategy.h"

class CSharpStrategy : public ILanguageStrategy {
   public:
    CSharpStrategy();

    CompilationResult compile(const QString &source, const QString &compiler,
                              const QString &flags) override;
    QString getHighlightingRules() override;
    QString getLanguageName() const override { return "C#"; }
    QString getFileExtension() const override { return ".cs"; }
    QStringList getRecommendedCompilers() const override;

   private:
    QString extractIL(const QString &asmPath);
};

#endif
