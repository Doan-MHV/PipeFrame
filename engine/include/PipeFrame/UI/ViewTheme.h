#pragma once
#include <PipeFrame/Foundation/MathTypes.h>
namespace pipeframe::ui {
template <typename ColorType, typename VectorType>
struct ThemeTokens {
    ColorType applicationBackground{24, 25, 28};
    ColorType surface{29, 32, 39};
    ColorType elevatedSurface{35, 39, 48};
    ColorType inputBackground{22, 24, 30};
    ColorType floatingSurface{27, 32, 42, 248};
    ColorType glassSurface{21, 29, 40, 222};
    ColorType scrim{8, 10, 14, 150};
    ColorType shadow{4, 7, 12, 105};

    ColorType border{63, 70, 84};
    ColorType subtleBorder{48, 53, 64};

    ColorType textPrimary{230, 233, 240};
    ColorType textSecondary{160, 167, 181};
    ColorType textDisabled{92, 98, 111};

    ColorType accent{66, 133, 214};
    ColorType accentHovered{79, 151, 236};
    ColorType accentPressed{51, 111, 184};

    ColorType controlNormal{37, 41, 51};
    ColorType controlHovered{48, 54, 67};
    ColorType controlPressed{61, 72, 94};
    ColorType controlDisabled{32, 35, 42};
    ColorType controlSelected{49, 113, 177};

    ColorType danger{212, 74, 74};
    ColorType warning{225, 155, 65};
    ColorType success{71, 181, 112};

    float spacing2 = 2.0f;
    float spacing4 = 4.0f;
    float spacing8 = 8.0f;
    float spacing12 = 12.0f;
    float spacing16 = 16.0f;
    float spacing24 = 24.0f;

    float smallControlHeight = 28.0f;
    float controlHeight = 36.0f;
    float largeControlHeight = 44.0f;

    float borderThickness = 1.0f;

    float radiusSmall = 4.0f;
    float radiusMedium = 8.0f;
    float radiusLarge = 12.0f;
    float radiusPill = 999.0f;

    VectorType shadowOffsetSmall{0.0f, 2.0f};
    VectorType shadowOffsetLarge{0.0f, 5.0f};

    float motionFast = 0.08f;
    float motionNormal = 0.16f;
    float motionSlow = 0.24f;

    unsigned int captionTextSize = 12;
    unsigned int bodyTextSize = 14;
    unsigned int headingTextSize = 16;

    static const ThemeTokens& Dark() {
        static const ThemeTokens theme;
        return theme;
    }
};

using ViewTheme = ThemeTokens<pipeframe::Color, pipeframe::Vector2f>;
}  // namespace pipeframe::ui
