# 1 "./src/strategies/CppStrategy.h"
#ifndef CPPSTRATEGY_H
#define CPPSTRATEGY_H

#include "ILanguageStrategy.h"

class CppStrategy : public ILanguageStrategy {
   public:
    CppStrategy();

    CompilationResult compile(const QString &source, const QString &compiler,
                              const QString &flags) override;
    QString getHighlightingRules() override;
    QString getLanguageName() const override { return "C++"; }
    QString getFileExtension() const override { return ".cpp"; }
    QStringList getRecommendedCompilers() const override;

   private:
    QString cleanAssembly(const QString &assembly);
};

#endif
