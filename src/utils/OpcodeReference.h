#ifndef OPCODEREFERENCE_H
#define OPCODEREFERENCE_H

#include <QMap>
#include <QString>

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
            .arg(mnemonic, architecture, summary, flagsAffected, syntaxExample);
    }
};

class OpcodeReference {
public:
    static OpcodeReference& instance();

    bool hasOpcode(const QString& mnemonic) const;
    OpcodeInfo getOpcodeInfo(const QString& mnemonic) const;

private:
    OpcodeReference();
    ~OpcodeReference() = default;
    OpcodeReference(const OpcodeReference&) = delete;
    OpcodeReference& operator=(const OpcodeReference&) = delete;

    void registerOpcode(const QString& mnemonic, const QString& arch,
                        const QString& summary, const QString& flags,
                        const QString& syntax);

    QMap<QString, OpcodeInfo> m_opcodes;
};

#endif // OPCODEREFERENCE_H
