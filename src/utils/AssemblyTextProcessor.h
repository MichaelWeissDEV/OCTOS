#ifndef ASSEMBLYTEXTPROCESSOR_H
#define ASSEMBLYTEXTPROCESSOR_H

#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

struct FilterSettings {
    bool showSegmentDirectives = false;
    bool showDataDirectives = false;
    bool showCfiDirectives = false;
    bool showMetadataLabels = false;
    bool showUnusedLabels = false;
    bool hideEmptyLabels = false;
    bool showDebugInfo = false;
    bool showComments = false;
    bool demangleIdentifiers = true;
};

struct ProcessedAssemblyResult {
    QString filteredText;
    QMap<int, QVector<int>> sourceToDisplay;
};

class AssemblyTextProcessor {
   public:
    static ProcessedAssemblyResult processAssembly(const QString& rawAssembly,
                                                   const FilterSettings& settings);

   private:
    static QSet<QString> findUsedLabels(const QStringList& lines);
    static bool isLabelDefinition(const QString& line);
    static QString labelNameFromDefinition(const QString& line);
    static bool isMetadataLabel(const QString& label);
    static bool isSegmentDirective(const QString& line);
    static bool isDataDirective(const QString& line);
    static bool isCfiDirective(const QString& line);
    static bool isDebugInfoDirective(const QString& line);
    static bool isInstruction(const QString& line);
    static bool hasUsefulContentAfter(const QStringList& lines, int currentIndex);
    static QString stripComments(const QString& line);
    static int extractSourceLine(const QString& line, int previousLine);
    static QString demangleText(const QString& text);
};

#endif  // ASSEMBLYTEXTPROCESSOR_H
