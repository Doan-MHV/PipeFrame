#ifndef PIPEFRAME_LABELED_TEXT_FIELD_H
#define PIPEFRAME_LABELED_TEXT_FIELD_H

#include <string>

#include <SFML/Graphics/Font.hpp>

#include <PipeFrame/Backend/SFML/UI/StackPanel.h>
#include <PipeFrame/Backend/SFML/UI/TextField.h>

class Label;

class LabeledTextField : public StackPanel {
public:
    LabeledTextField(
        const sf::Font &font,
        const std::string &caption
    );

    void SetCaptionAbove();

    void SetCaption(
        const std::string &caption
    );

    void SetValue(
        const std::string &value
    );

    [[nodiscard]]
    const std::string &GetValue() const;

    void SetOnValueCommitted(
        TextField::ValueCommittedCallback callback
    );

    [[nodiscard]]
    bool IsEditing() const;

protected:
    void OnEnabledChanged() override;

private:
    Label *captionLabel{nullptr};
    TextField *field{nullptr};
};

#endif