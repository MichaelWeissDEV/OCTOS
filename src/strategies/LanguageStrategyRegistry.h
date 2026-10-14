#ifndef LANGUAGESTRATEGYREGISTRY_H
#define LANGUAGESTRATEGYREGISTRY_H

#include <memory>
#include <QMap>
#include <QString>
#include "ILanguageStrategy.h"
#include "../core/CompilerEnums.h"

class LanguageStrategyRegistry {
public:
    static LanguageStrategyRegistry& instance();

    const ILanguageStrategy* getStrategy(Language lang, const QString& compilerImage = QString()) const;

    bool supportsSyntaxFlavor(Language lang) const;

private:
    LanguageStrategyRegistry();
    ~LanguageStrategyRegistry() = default;
    LanguageStrategyRegistry(const LanguageStrategyRegistry&) = delete;
    LanguageStrategyRegistry& operator=(const LanguageStrategyRegistry&) = delete;

    std::unique_ptr<ILanguageStrategy> m_gccCppStrategy;
    std::unique_ptr<ILanguageStrategy> m_gccCStrategy;
    std::unique_ptr<ILanguageStrategy> m_clangCppStrategy;
    std::unique_ptr<ILanguageStrategy> m_clangCStrategy;
    std::unique_ptr<ILanguageStrategy> m_rustStrategy;
    std::unique_ptr<ILanguageStrategy> m_javaStrategy;
    std::unique_ptr<ILanguageStrategy> m_pythonStrategy;
    std::unique_ptr<ILanguageStrategy> m_csharpStrategy;
    std::unique_ptr<ILanguageStrategy> m_adaStrategy;
};

#endif // LANGUAGESTRATEGYREGISTRY_H
