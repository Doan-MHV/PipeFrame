#ifndef PIPEFRAME_UI_SURFACE_H
#define PIPEFRAME_UI_SURFACE_H

#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <PipeFrame/Backend/SFML/UI/UITheme.h>

enum class SurfaceVariant {
    Base,
    Elevated,
    Floating,
    Glass,
};

class Surface : public Panel {
  public:
    explicit Surface(SurfaceVariant variant = SurfaceVariant::Base,
                     const UITheme &theme = UITheme::Dark());

    void SetVariant(SurfaceVariant variant, const UITheme &theme = UITheme::Dark());
    SurfaceVariant GetVariant() const;

  private:
    SurfaceVariant variant = SurfaceVariant::Base;
};

class Card final : public Surface {
  public:
    explicit Card(const UITheme &theme = UITheme::Dark())
        : Surface(SurfaceVariant::Elevated, theme) {}
};

#endif
