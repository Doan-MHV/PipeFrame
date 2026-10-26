#include <PipeFrame/Backend/SFML/UI/Surface.h>

Surface::Surface(const SurfaceVariant newVariant, const UITheme &theme) { SetVariant(newVariant, theme); }

void Surface::SetVariant(const SurfaceVariant newVariant, const UITheme &theme) {
    variant = newVariant;
    SetOutlineThickness(theme.borderThickness);
    SetOutlineColor(theme.subtleBorder);

    switch (variant) {
    case SurfaceVariant::Base:
        SetFillColor(theme.surface);
        SetCornerRadius(theme.radiusSmall);
        SetShadowColor(sf::Color::Transparent);
        SetShadowOffset({0.0f, 0.0f});
        break;
    case SurfaceVariant::Elevated:
        SetFillColor(theme.elevatedSurface);
        SetCornerRadius(theme.radiusMedium);
        SetShadowColor(theme.shadow);
        SetShadowOffset(theme.shadowOffsetSmall);
        break;
    case SurfaceVariant::Floating:
        SetFillColor(theme.floatingSurface);
        SetCornerRadius(theme.radiusLarge);
        SetShadowColor(theme.shadow);
        SetShadowOffset(theme.shadowOffsetLarge);
        break;
    case SurfaceVariant::Glass:
        SetFillColor(theme.glassSurface);
        SetCornerRadius(theme.radiusLarge);
        SetShadowColor(theme.shadow);
        SetShadowOffset(theme.shadowOffsetSmall);
        break;
    }
}

SurfaceVariant Surface::GetVariant() const { return variant; }
