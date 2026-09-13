#ifndef PIPEFRAME_RESOURCES_IMAGE_DATA_H
#define PIPEFRAME_RESOURCES_IMAGE_DATA_H

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include <PipeFrame/Foundation/MathTypes.h>

namespace pipeframe {

class ImageData {
  public:
    ImageData() = default;
    explicit ImageData(Vector2u size, Color fill = {});

    void Resize(Vector2u size, Color fill = {});

    [[nodiscard]] Vector2u Size() const;
    [[nodiscard]] bool Empty() const;
    [[nodiscard]] Color Pixel(Vector2u position) const;
    bool SetPixel(Vector2u position, Color color);

    [[nodiscard]] const std::vector<Color> &Pixels() const;
    [[nodiscard]] std::vector<Color> &Pixels();

  private:
    [[nodiscard]] std::size_t Offset(Vector2u position) const;

    Vector2u size{};
    std::vector<Color> pixels;
};

bool LoadImageData(
    const std::filesystem::path &filePath,
    ImageData &image,
    std::string &errorMessage
);

bool SaveImageData(
    const std::filesystem::path &filePath,
    const ImageData &image,
    std::string &errorMessage
);

} // namespace pipeframe

#endif
