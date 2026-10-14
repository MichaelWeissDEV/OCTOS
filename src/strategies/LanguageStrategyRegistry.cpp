#include "LanguageStrategyRegistry.h"
#include "GccStrategy.h"
#include "ClangStrategy.h"
#include "RustStrategy.h"
#include "JavaStrategy.h"
#include "PythonStrategy.h"
#include "CSharpStrategy.h"
#include "AdaStrategy.h"

LanguageStrategyRegistry& LanguageStrategyRegistry::instance() {
    static LanguageStrategyRegistry inst;
    return inst;
}

LanguageStrategyRegistry::LanguageStrategyRegistry()
    : m_gccCppStrategy(std::make_unique<GccStrategy>(Language::Cpp)),
      m_gccCStrategy(std::make_unique<GccStrategy>(Language::C)),
      m_clangCppStrategy(std::make_unique<ClangStrategy>(Language::Cpp)),
      m_clangCStrategy(std::make_unique<ClangStrategy>(Language::C)),
      m_rustStrategy(std::make_unique<RustStrategy>()),
      m_javaStrategy(std::make_unique<JavaStrategy>()),
      m_pythonStrategy(std::make_unique<PythonStrategy>()),
      m_csharpStrategy(std::make_unique<CSharpStrategy>()),
      m_adaStrategy(std::make_unique<AdaStrategy>()) {
}

const ILanguageStrategy* LanguageStrategyRegistry::getStrategy(Language lang,
                                                               const QString& compilerImage) const {
    QString imageLower = compilerImage.toLower();
    bool isClang = imageLower.contains("clang");

    switch (lang) {
        case Language::Cpp:
            return isClang ? m_clangCppStrategy.get() : m_gccCppStrategy.get();
        case Language::C:
            return isClang ? m_clangCStrategy.get() : m_gccCStrategy.get();
        case Language::Rust:
            return m_rustStrategy.get();
        case Language::Java:
            return m_javaStrategy.get();
        case Language::Python:
            return m_pythonStrategy.get();
        case Language::CSharp:
            return m_csharpStrategy.get();
        case Language::Ada:
            return m_adaStrategy.get();
    }
    return m_gccCppStrategy.get();
}

bool LanguageStrategyRegistry::supportsSyntaxFlavor(Language lang) const {
    switch (lang) {
        case Language::Cpp:
        case Language::C:
        case Language::Rust:
        case Language::Ada:
            return true;
        case Language::Java:
        case Language::Python:
        case Language::CSharp:
            return false;
    }
    return false;
}
