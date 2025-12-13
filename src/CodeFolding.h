# 1 "./src/CodeFolding.h"
#ifndef CODEFOLDING_H
#define CODEFOLDING_H

#include <QList>
#include <QMap>
#include <QRegularExpression>
#include <QString>

struct FoldRegion {
    int startLine;
    int endLine;
    QString type;
    bool isFolded;
};

class CodeFolding {
   public:
    static QList<FoldRegion> extractFoldRegions(const QString &source, const QString &language) {
        QList<FoldRegion> regions;

        if (language == "C++" || language == "C") {
            regions = extractCppFoldRegions(source);
        } else if (language == "Java") {
            regions = extractJavaFoldRegions(source);
        } else if (language == "Python") {
            regions = extractPythonFoldRegions(source);
        }

        return regions;
    }

   private:
    static QList<FoldRegion> extractCppFoldRegions(const QString &source) {
        QList<FoldRegion> regions;
        QStringList lines = source.split('\n');

        int braceDepth = 0;
        int braceStartLine = -1;

        for (int i = 0; i < lines.size(); ++i) {
            const QString &line = lines[i];

            if (line.contains(QRegularExpression(
                    "^\\s*(class|struct|namespace|void|int|bool|auto)\\s+\\w+.*\\{")) ||
                line.contains(
                    QRegularExpression("^\\s*(if|for|while|switch)\\s*\\(.*\\)\\s*\\{"))) {
                braceStartLine = i;
                braceDepth = 1;
            } else if (line.contains('{')) {
                braceDepth++;
            } else if (line.contains('}')) {
                braceDepth--;
                if (braceDepth == 0 && braceStartLine != -1) {
                    FoldRegion region;
                    region.startLine = braceStartLine;
                    region.endLine = i;
                    region.type = "block";
                    region.isFolded = false;
                    regions.append(region);
                    braceStartLine = -1;
                }
            }
        }

        return regions;
    }

    static QList<FoldRegion> extractJavaFoldRegions(const QString &source) {
        return extractCppFoldRegions(source);
    }

    static QList<FoldRegion> extractPythonFoldRegions(const QString &source) {
        QList<FoldRegion> regions;
        QStringList lines = source.split('\n');

        for (int i = 0; i < lines.size(); ++i) {
            const QString &line = lines[i];
            int indent = 0;

            for (const QChar &ch : line) {
                if (ch == ' ')
                    indent++;
                else if (ch == '\t')
                    indent += 4;
                else
                    break;
            }

            if (line.trimmed().startsWith("def ") || line.trimmed().startsWith("class ")) {
                int endLine = i + 1;
                while (endLine < lines.size()) {
                    int nextIndent = 0;
                    const QString &nextLine = lines[endLine];

                    for (const QChar &ch : nextLine) {
                        if (ch == ' ')
                            nextIndent++;
                        else if (ch == '\t')
                            nextIndent += 4;
                        else
                            break;
                    }

                    if (!nextLine.trimmed().isEmpty() && nextIndent <= indent) break;

                    endLine++;
                }

                if (endLine > i + 1) {
                    FoldRegion region;
                    region.startLine = i;
                    region.endLine = endLine - 1;
                    region.type = line.trimmed().startsWith("class") ? "class" : "function";
                    region.isFolded = false;
                    regions.append(region);
                }
            }
        }

        return regions;
    }
};

#endif
