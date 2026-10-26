#ifndef PIPEFRAME_SPACER_H
#define PIPEFRAME_SPACER_H

#include <PipeFrame/Backend/SFML/UI/Widget.h>

class Spacer final : public Widget {
public:
    Spacer() { SetSizePolicy(SizePolicy::Stretch, SizePolicy::Stretch); }

protected:
    void OnRender(sf::RenderTarget& target) const override { (void)target; }
};

#endif
