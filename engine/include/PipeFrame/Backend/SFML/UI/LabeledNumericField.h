#ifndef PIPEFRAME_LABELED_NUMERIC_FIELD_H
#define PIPEFRAME_LABELED_NUMERIC_FIELD_H

#include <string>

#include <SFML/Graphics/Font.hpp>

#include <PipeFrame/Backend/SFML/UI/NumericField.h>
#include <PipeFrame/Backend/SFML/UI/StackPanel.h>

class Label;

class LabeledNumericField : public StackPanel {
public:
    LabeledNumericField(
        const sf::Font &font,
        const std::string &caption);

    void SetCaptionAbove();

    void SetCaption(const std::string &caption);

    void SetValue(float value);
    float GetValue() const;

    void SetOnValueCommitted(
        NumericField::ValueCommittedCallback callback);

    bool IsEditing() const;

protected:
    void OnEnabledChanged() override;

private:
    Label *captionLabel = nullptr;
    NumericField *field = nullptr;
};

#endif