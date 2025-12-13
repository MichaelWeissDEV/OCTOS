# 1 "./src/ui/Theme.h"
#ifndef THEME_H
#define THEME_H

#include <QString>

enum class ThemeType { Dark, Light, SolarizedDark, SolarizedLight, Dracula, Nord };

class Theme {
   public:
    static QString getDarkStylesheet();
    static QString getLightStylesheet();
    static QString getSolarizedDarkStylesheet();
    static QString getSolarizedLightStylesheet();
    static QString getDraculaStylesheet();
    static QString getNordStylesheet();

    static QString getStylesheet(ThemeType theme);
    static QStringList getAvailableThemes();
};

#endif
