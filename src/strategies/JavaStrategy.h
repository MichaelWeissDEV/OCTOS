# 1 "./src/strategies/JavaStrategy.h"
#ifndef JAVASTRATEGY_H
#define JAVASTRATEGY_H

#include "ILanguageStrategy.h"

class JavaStrategy : public ILanguageStrategy {
   public:
    JavaStrategy();

    CompilationResult compile(const QString &source, const QString &compiler,
                              const QString &flags) override;
    QString getHighlightingRules() override;
    QString getLanguageName() const override { return "Java"; }
    QString getFileExtension() const override { return ".java"; }
    QStringList getRecommendedCompilers() const override;

   private:
    QString extractClassName(const QString &source);
};

#endif
