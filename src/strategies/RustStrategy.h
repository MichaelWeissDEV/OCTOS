# 1 "./src/strategies/RustStrategy.h"
#ifndef RUSTSTRATEGY_H
#define RUSTSTRATEGY_H

#include "ILanguageStrategy.h"

class RustStrategy : public ILanguageStrategy {
   public:
    RustStrategy();

    CompilationResult compile(const QString &source, const QString &compiler,
                              const QString &flags) override;
    bool supportsDualView() const override { return false; }
    QStringList getRecommendedCompilers() const override;
    void setDualViewMode(bool enabled) override;
    QString getHighlightingRules() override;
    QString getLanguageName() const override;
    QString getFileExtension() const override;

   private:
    CompilationResult compileToAssembly(const QString &source, const QString &flags);
    QString parseRustOutput(const QString &output);
};

#endif
