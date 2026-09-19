#pragma once
#include <PipeFrame/Environment/TilemapEdit.h>
#include <PipeFrame/Project/ComponentSchema.h>

#include <functional>
#include <memory>

namespace pipeframe {
// Effects own only domain logic/settings. TilemapPaintTool owns gestures and transactions.
class BrushTool : public EnvironmentBrush {
public:
    virtual SceneComponentTypeDescriptor SettingsSchema() const = 0;
    virtual PropertyMap Settings() const = 0;
    virtual bool SetSetting(const std::string& key, const PropertyValue& value, std::string& error) = 0;
    virtual double InitialData(const Tilemap2D&, GridCoordinate) const { return 0; }
    virtual double DataMinimum() const { return 0; }
    virtual double DataMaximum() const { return 1; }
};
template <class Derived>
class SchemaBrush : public BrushTool {
public:
    SceneComponentTypeDescriptor SettingsSchema() const override { return Derived::Schema().Describe(); }
    PropertyMap Settings() const override {
        return Derived::Schema().Serialize(static_cast<const Derived&>(*this)).properties;
    }
    bool SetSetting(const std::string& key, const PropertyValue& value, std::string& error) override {
        return Derived::Schema().Apply(static_cast<Derived&>(*this), {{key, value}}, error);
    }
};
// Reusable zero-value effect for the editor's erase gesture on any numeric layer.
class EraseDataBrush final : public EnvironmentBrush {
public:
    explicit EraseDataBrush(std::string target) : target(std::move(target)) {}
    std::string_view DataTarget() const override { return target; }
    double PaintData(GridCoordinate, double) const override { return 0; }

private:
    std::string target;
};
struct BrushToolDescriptor {
    std::string id, label, category;
    std::function<std::shared_ptr<BrushTool>()> create;
};
}  // namespace pipeframe
