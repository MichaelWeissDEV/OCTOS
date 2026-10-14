#ifndef OPCODEREFERENCE_H
#define OPCODEREFERENCE_H

#include <QMap>
#include <QString>
#include <QStringList>

struct OpcodeInfo {
    QString mnemonic;
    QString architecture;
    QString summary;
    QString flagsAffected;
    QString syntaxExample;

    QString toFormattedTooltip() const {
        return QString("<b>%1</b> (%2)<br>"
                       "<b>Summary:</b> %3<br>"
                       "<b>Flags:</b> %4<br>"
                       "<b>Syntax:</b> <code>%5</code>")
            .arg(mnemonic.toHtmlEscaped(),
                 architecture.toHtmlEscaped(),
                 summary.toHtmlEscaped(),
                 flagsAffected.toHtmlEscaped(),
                 syntaxExample.toHtmlEscaped());
    }
};

class OpcodeReference {
public:
    static OpcodeReference& instance();

    bool hasOpcode(const QString& mnemonic, const QString& arch = QString()) const;
    OpcodeInfo getOpcodeInfo(const QString& mnemonic, const QString& arch = QString()) const;

    QStringList supportedArchitectures() const;

    // Accurate mnemonic extraction that rejects comments, operands, and directives
    static bool extractMnemonicAtColumn(const QString& lineText, int column,
                                        QString* outMnemonic, QString* detectedArch = nullptr);

private:
    OpcodeReference();
    ~OpcodeReference() = default;
    OpcodeReference(const OpcodeReference&) = delete;
    OpcodeReference& operator=(const OpcodeReference&) = delete;

    void registerOpcode(const QString& mnemonic, const QString& arch,
                         const QString& summary, const QString& flags,
                         const QString& syntax);

    // Architecture -> (mnemonic -> OpcodeInfo)
    QMap<QString, QMap<QString, OpcodeInfo>> m_archOpcodes;
    // Flat fallback map for convenience
    QMap<QString, OpcodeInfo> m_allOpcodes;
};

#endif // OPCODEREFERENCE_H
