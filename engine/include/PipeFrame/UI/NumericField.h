#ifndef PIPEFRAME_NUMERIC_FIELD_H
#define PIPEFRAME_NUMERIC_FIELD_H

#include <functional>
#include <string>

#include <SFML/Graphics/Text.hpp>

#include <PipeFrame/UI/Panel.h>

class NumericField : public Panel {
  public:
    using ValueCommittedCallback = std::function<void(float)>;

    explicit NumericField(const sf::Font &font);

    void SetValue(float newValue);
    float GetValue() const;

    void SetOnValueCommitted(ValueCommittedCallback callback);

  protected:
    void OnRender(sf::RenderTarget &target) const override;
    void OnGeometryChanged() override;
    bool OnEvent(const sf::Event &event) override;

    void OnKeyboardFocusGained() override;
    void OnKeyboardFocusLost() override;

    void OnEnabledChanged() override;

  private:
    void Commit();
    void CancelEditing();
    void RefreshText();
    void RefreshTextPosition();
    void RefreshVisual();

    static std::string FormatValue(float value);

    sf::Text text;

    float value = 0.0f;
    std::string editBuffer;

    bool replaceOnNextText = false;

    ValueCommittedCallback onValueCommitted;
};

#endif