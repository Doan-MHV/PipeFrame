#ifndef PIPEFRAME_LABELED_NUMERIC_FIELD_H
#define PIPEFRAME_LABELED_NUMERIC_FIELD_H

#include <string>

#include <SFML/Graphics/Font.hpp>

#include <PipeFrame/UI/NumericField.h>
#include <PipeFrame/UI/StackPanel.h>

class LabeledNumericField : public StackPanel {
  public:
    LabeledNumericField(const sf::Font &font, const std::string &caption);

    void SetValue(float value);
    float GetValue() const;

    void SetOnValueCommitted(NumericField::ValueCommittedCallback callback);

    bool IsEditing() const;

  protected:
    void OnEnabledChanged() override;

  private:
    NumericField *field = nullptr;
};

#endif