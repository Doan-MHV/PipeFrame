#ifndef PIPEFRAME_TEXT_FIELD_H
#define PIPEFRAME_TEXT_FIELD_H

#include <functional>
#include <string>

#include <SFML/Graphics/Text.hpp>

#include <PipeFrame/Backend/SFML/UI/Panel.h>

class TextField : public Panel {
public:
    using ValueCommittedCallback =
        std::function<void(const std::string &)>;

    explicit TextField(const sf::Font &font);

    void SetValue(const std::string &newValue);

    [[nodiscard]]
    const std::string &GetValue() const;

    void SetOnValueCommitted(
        ValueCommittedCallback callback
    );

protected:
    void OnRender(
        sf::RenderTarget &target
    ) const override;

    void OnGeometryChanged() override;

    bool OnEvent(
        const sf::Event &event
    ) override;

    void OnKeyboardFocusGained() override;
    void OnKeyboardFocusLost() override;
    void OnEnabledChanged() override;

private:
    void Commit();
    void CancelEditing();

    void RefreshText();
    void RefreshTextPosition();
    void RefreshVisual();

    [[nodiscard]]
    std::string GetDisplayText() const;

    sf::Text text;

    std::string value;
    std::string editBuffer;

    bool replaceOnNextText{false};

    ValueCommittedCallback onValueCommitted;
};

#endif