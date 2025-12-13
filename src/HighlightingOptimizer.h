# 1 "./src/HighlightingOptimizer.h"
#ifndef HIGHLIGHTINGOPTIMIZER_H
#define HIGHLIGHTINGOPTIMIZER_H

#include <QString>

class HighlightingOptimizer {
   public:
    static bool shouldUseLazyHighlighting(int fileSize) { return fileSize > 100 * 1024; }

    static int getHighlightingBlockSize() { return 50 * 1024; }

    static int getMaxVisibleLines() { return 500; }

    static int getOptimalTabStopDistance(const QString &content) {
        int maxIndentLevel = 0;
        int currentIndent = 0;

        for (const QChar &ch : content) {
            if (ch == '\t') {
                currentIndent++;
                maxIndentLevel = qMax(maxIndentLevel, currentIndent);
            } else if (ch == '\n') {
                currentIndent = 0;
            }
        }

        return 30 + (maxIndentLevel * 5);
    }
};

#endif
