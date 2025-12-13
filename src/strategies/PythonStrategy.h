# 1 "./src/strategies/PythonStrategy.h"
#ifndef PYTHONSTRATEGY_H
#define PYTHONSTRATEGY_H

#include "ILanguageStrategy.h"

class PythonStrategy : public ILanguageStrategy {
   public:
    PythonStrategy();

    CompilationResult compile(const QString &source, const QString &compiler,
                              const QString &flags) override;
    QString getHighlightingRules() override;
    QString getLanguageName() const override { return "Python"; }
    QString getFileExtension() const override { return ".py"; }
    QStringList getRecommendedCompilers() const override;

    bool supportsDualView() const override { return true; }
    bool isDualViewActive() const override { return m_dualViewEnabled; }
    void setDualViewMode(bool enabled) override { m_dualViewEnabled = enabled; }

   private:
    CompilationResult compileBytecode(const QString &source, const QString &compiler);
    CompilationResult compileNumba(const QString &source, const QString &compiler);
    QString extractNumbaCode(const QString &source);
    void createNumbaHelper();

    bool m_dualViewEnabled = false;
    QString m_numbaHelperPath;
};

#endif
