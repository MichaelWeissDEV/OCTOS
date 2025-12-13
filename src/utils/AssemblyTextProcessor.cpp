#include "AssemblyTextProcessor.h"

#include <QHash>
#include <QProcess>
#include <QRegularExpression>

namespace {
const QRegularExpression kLocRegex(R"(^\.loc\s+\d+\s+(\d+))");
const QRegularExpression kLabelDefinitionRegex(R"(^([A-Za-z_.$][\w.$@]*):)");
const QRegularExpression kLabelReferenceRegex(R"((\.?[A-Za-z_][\w.$@]*))");
const QRegularExpression kMangledNameRegex(R"(_Z[A-Za-z0-9_$.@]*)");

bool isBranchMnemonic(const QString& token) {
    static const QSet<QString> mnemonics = {
        "call", "jmp",  "je",   "jne",  "ja",  "jb",  "jg",   "jl",    "jae",   "jbe",    "jge",
        "jle",  "jo",   "jno",  "js",   "jns", "jp",  "jnp",  "jz",    "jnz",   "jc",     "jnc",
        "jnae", "jnbe", "jnge", "jnle", "jpe", "jpo", "loop", "loope", "loopz", "loopne", "loopnz"};
    return mnemonics.contains(token.toLower());
}

}  // namespace

QSet<QString> AssemblyTextProcessor::findUsedLabels(const QStringList& lines) {
    QSet<QString> usedLabels;
    usedLabels.insert("main");

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        QStringList tokens = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (tokens.isEmpty()) {
            continue;
        }

        QString mnemonic = tokens.first();
        if (isBranchMnemonic(mnemonic)) {
            for (int i = 1; i < tokens.size(); ++i) {
                QString candidate = tokens[i];
                candidate.remove(',');
                candidate.remove('(');
                candidate.remove(')');
                candidate.remove('[');
                candidate.remove(']');
                candidate.remove('+');
                candidate.remove('-');
                if (candidate.endsWith('f') || candidate.endsWith('b')) {
                    continue;
                }
                QRegularExpressionMatch refMatch = kLabelReferenceRegex.match(candidate);
                if (refMatch.hasMatch()) {
                    QString label = refMatch.captured(1);
                    if (!label.isEmpty()) {
                        usedLabels.insert(label);
                    }
                }
            }
        }

        QRegularExpressionMatchIterator it = kLabelReferenceRegex.globalMatch(trimmed);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            QString label = match.captured(1);
            if (label.startsWith(".L")) {
                usedLabels.insert(label);
            }
        }
    }

    return usedLabels;
}

bool AssemblyTextProcessor::isLabelDefinition(const QString& line) {
    return kLabelDefinitionRegex.match(line).hasMatch();
}

QString AssemblyTextProcessor::labelNameFromDefinition(const QString& line) {
    QRegularExpressionMatch match = kLabelDefinitionRegex.match(line);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return {};
}

bool AssemblyTextProcessor::isMetadataLabel(const QString& label) {
    if (!label.startsWith('.')) {
        return false;
    }
    static const QStringList markers = {"LFB", "LFE", "LASF", "LL", "LVL"};
    for (const QString& marker : markers) {
        if (label.contains(marker)) {
            return true;
        }
    }
    return false;
}

bool AssemblyTextProcessor::isSegmentDirective(const QString& line) {
    QString trimmed = line.trimmed();
    static const QStringList directives = {
        ".file",   ".globl", ".type",    ".size",  ".ident", ".section",      ".text",      ".data",
        ".rodata", ".bss",   ".p2align", ".align", ".weak",  ".intel_syntax", ".att_syntax"};
    for (const QString& directive : directives) {
        if (trimmed.startsWith(directive)) {
            return true;
        }
    }
    return false;
}

bool AssemblyTextProcessor::isDataDirective(const QString& line) {
    QString trimmed = line.trimmed();
    return trimmed.startsWith(".value") || trimmed.startsWith(".long") ||
           trimmed.startsWith(".quad") || trimmed.startsWith(".word") ||
           trimmed.startsWith(".short") || trimmed.startsWith(".byte") ||
           trimmed.startsWith(".string") || trimmed.startsWith(".ascii") ||
           trimmed.startsWith(".asciz") || trimmed.startsWith(".uleb128") ||
           trimmed.startsWith(".sleb128") || trimmed.startsWith(".zero");
}

bool AssemblyTextProcessor::isCfiDirective(const QString& line) {
    return line.trimmed().startsWith(".cfi_");
}

bool AssemblyTextProcessor::isDebugInfoDirective(const QString& line) {
    QString trimmed = line.trimmed();
    if (trimmed.startsWith(".loc")) {
        return true;
    }
    return trimmed.startsWith(".Ldebug") || trimmed.startsWith(".LLSD") ||
           trimmed.startsWith(".LEHB") || trimmed.startsWith(".LEHE");
}

bool AssemblyTextProcessor::isInstruction(const QString& line) {
    QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    if (trimmed.startsWith('.')) {
        return false;
    }
    if (trimmed.endsWith(':')) {
        return false;
    }
    if (trimmed.startsWith("#") || trimmed.startsWith(";")) {
        return false;
    }
    return true;
}

bool AssemblyTextProcessor::hasUsefulContentAfter(const QStringList& lines, int currentIndex) {
    for (int i = currentIndex + 1; i < lines.size(); ++i) {
        QString trimmed = lines[i].trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (trimmed.startsWith('#') || trimmed.startsWith(';')) {
            continue;
        }
        if (isLabelDefinition(trimmed)) {
            return false;
        }

        return true;
    }

    return false;
}

QString AssemblyTextProcessor::stripComments(const QString& line) {
    int hashIndex = line.indexOf('#');
    int semicolonIndex = line.indexOf(';');
    int cutIndex = -1;

    if (hashIndex >= 0 && semicolonIndex >= 0) {
        cutIndex = qMin(hashIndex, semicolonIndex);
    } else if (hashIndex >= 0) {
        cutIndex = hashIndex;
    } else if (semicolonIndex >= 0) {
        cutIndex = semicolonIndex;
    }

    if (cutIndex >= 0) {
        QString without = line.left(cutIndex);
        without.replace(QRegularExpression("\\s+$"), "");
        return without;
    }
    return line;
}

int AssemblyTextProcessor::extractSourceLine(const QString& line, int previousLine) {
    QRegularExpressionMatch match = kLocRegex.match(line.trimmed());
    if (match.hasMatch()) {
        bool ok = false;
        int value = match.captured(1).toInt(&ok);
        if (ok && value > 0) {
            return value;
        }
    }
    return previousLine;
}

QString AssemblyTextProcessor::demangleText(const QString& text) {
    QRegularExpressionMatchIterator iterator = kMangledNameRegex.globalMatch(text);
    QSet<QString> unique;
    while (iterator.hasNext()) {
        unique.insert(iterator.next().captured(0));
    }

    if (unique.isEmpty()) {
        return text;
    }

    QStringList names = unique.values();
    QProcess process;
    process.start("c++filt");
    if (!process.waitForStarted(1000)) {
        return text;
    }

    QByteArray payload = names.join('\n').toUtf8();
    payload.append('\n');
    process.write(payload);
    process.closeWriteChannel();
    if (!process.waitForFinished(2000)) {
        return text;
    }

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList demangled = output.split('\n');
    QHash<QString, QString> mapping;
    for (int i = 0; i < names.size() && i < demangled.size(); ++i) {
        mapping.insert(names[i], demangled[i].trimmed());
    }

    QString result = text;
    for (auto it = mapping.constBegin(); it != mapping.constEnd(); ++it) {
        if (!it.value().isEmpty()) {
            result.replace(it.key(), it.value());
        }
    }
    return result;
}

ProcessedAssemblyResult AssemblyTextProcessor::processAssembly(const QString& rawAssembly,
                                                               const FilterSettings& settings) {
    ProcessedAssemblyResult result;
    QStringList lines = rawAssembly.split('\n');

    QSet<QString> usedLabels;
    if (!settings.showUnusedLabels) {
        usedLabels = findUsedLabels(lines);
    }

    int currentSourceLine = -1;
    int displayIndex = 0;
    QStringList filteredLines;
    bool previousBlank = false;
    bool inDataSegment = false;

    for (int idx = 0; idx < lines.size(); ++idx) {
        const QString& rawLine = lines[idx];
        QString trimmed = rawLine.trimmed();

        if (trimmed.startsWith(".section")) {
            QString lower = trimmed.toLower();
            bool sectionIsData = lower.contains(".data") || lower.contains("rodata") ||
                                 lower.contains("sdata") || lower.contains("const") ||
                                 lower.contains("literal") || lower.contains("bss");
            bool sectionIsText = lower.contains(".text") || lower.contains(" text");
            if (sectionIsData) {
                inDataSegment = true;
            } else if (sectionIsText) {
                inDataSegment = false;
            }
            if (!settings.showSegmentDirectives) {
                continue;
            }
        } else if (trimmed.startsWith(".text")) {
            inDataSegment = false;
            if (!settings.showSegmentDirectives) {
                continue;
            }
        } else if (trimmed.startsWith(".data") || trimmed.startsWith(".rodata") ||
                   trimmed.startsWith(".sdata") || trimmed.startsWith(".bss") ||
                   trimmed.startsWith(".tdata") || trimmed.startsWith(".tbss")) {
            inDataSegment = true;
            if (!settings.showSegmentDirectives) {
                continue;
            }
        }

        if (trimmed.startsWith(".loc")) {
            currentSourceLine = extractSourceLine(trimmed, currentSourceLine);
            if (!settings.showDebugInfo) {
                continue;
            }
        }

        if (isCfiDirective(trimmed) && !settings.showCfiDirectives) {
            continue;
        }

        if (isDebugInfoDirective(trimmed) && !trimmed.startsWith(".loc")) {
            if (!settings.showDebugInfo) {
                continue;
            }
        }

        if (isSegmentDirective(trimmed)) {
            if (!settings.showSegmentDirectives) {
                continue;
            }
        }

        if (isDataDirective(trimmed)) {
            bool keep = settings.showDataDirectives || inDataSegment;
            if (!keep) {
                continue;
            }
        }

        if (isLabelDefinition(trimmed)) {
            QString labelName = labelNameFromDefinition(trimmed);
            if (!settings.showMetadataLabels && isMetadataLabel(labelName)) {
                continue;
            }
            if (settings.hideEmptyLabels && !hasUsefulContentAfter(lines, idx)) {
                continue;
            }
            bool keepLabel = settings.showUnusedLabels || !labelName.startsWith('.') ||
                             usedLabels.contains(labelName);
            if (!keepLabel) {
                continue;
            }
        }

        QString processedLine = rawLine;
        if (!settings.showComments) {
            processedLine = stripComments(processedLine);
        }
        if (processedLine.trimmed().isEmpty()) {
            if (previousBlank) {
                continue;
            }
            previousBlank = true;
            filteredLines.append(QString());
            ++displayIndex;
            continue;
        }
        previousBlank = false;

        processedLine.replace(QRegularExpression("\\s+$"), "");
        filteredLines.append(processedLine);

        if (currentSourceLine > 0 && isInstruction(processedLine)) {
            result.sourceToDisplay[currentSourceLine].append(displayIndex);
        }

        ++displayIndex;
    }

    QString finalText = filteredLines.join('\n');
    if (settings.demangleIdentifiers) {
        finalText = demangleText(finalText);
    }

    result.filteredText = finalText;

    return result;
}
