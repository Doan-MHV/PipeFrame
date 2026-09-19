#include <PipeFrame/Resources/ImageData.h>

#include <limits>

#include <SFML/Graphics/Image.hpp>

namespace pipeframe {

ImageData::ImageData(const Vector2u imageSize, const Color fill) { Resize(imageSize, fill); }

void ImageData::Resize(const Vector2u imageSize, const Color fill) {
    size = imageSize;

    if (size.x == 0 || size.y == 0 ||
        static_cast<std::size_t>(size.x) > std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(size.y)) {
        size = {};
        pixels.clear();
        return;
    }

    pixels.assign(static_cast<std::size_t>(size.x) * size.y, fill);
}

Vector2u ImageData::Size() const { return size; }

bool ImageData::Empty() const { return pixels.empty(); }

Color ImageData::Pixel(const Vector2u position) const {
    if (position.x >= size.x || position.y >= size.y) {
        return {};
    }

    return pixels[Offset(position)];
}

bool ImageData::SetPixel(const Vector2u position, const Color color) {
    if (position.x >= size.x || position.y >= size.y) {
        return false;
    }

    pixels[Offset(position)] = color;
    return true;
}

const std::vector<Color> &ImageData::Pixels() const { return pixels; }

std::vector<Color> &ImageData::Pixels() { return pixels; }

std::size_t ImageData::Offset(const Vector2u position) const {
    return static_cast<std::size_t>(position.y) * size.x + position.x;
}

bool LoadImageData(const std::filesystem::path &filePath, ImageData &image, std::string &errorMessage) {
    errorMessage.clear();

    if (filePath.empty()) {
        errorMessage = "Image path cannot be empty.";
        return false;
    }

    sf::Image backendImage;

    if (!backendImage.loadFromFile(filePath)) {
        errorMessage = "Unable to load image: " + filePath.string();
        return false;
    }

    const sf::Vector2u backendSize = backendImage.getSize();
    image.Resize({backendSize.x, backendSize.y});

    for (std::uint32_t y = 0; y < backendSize.y; ++y) {
        for (std::uint32_t x = 0; x < backendSize.x; ++x) {
            const sf::Color pixel = backendImage.getPixel({x, y});
            image.SetPixel({x, y}, {pixel.r, pixel.g, pixel.b, pixel.a});
        }
    }

    return true;
}

bool SaveImageData(const std::filesystem::path &filePath, const ImageData &image, std::string &errorMessage) {
    errorMessage.clear();

    if (filePath.empty()) {
        errorMessage = "Image path cannot be empty.";
        return false;
    }

    if (image.Empty()) {
        errorMessage = "Image cannot be empty.";
        return false;
    }

    const Vector2u size = image.Size();
    sf::Image backendImage({size.x, size.y}, sf::Color::Transparent);

    for (std::uint32_t y = 0; y < size.y; ++y) {
        for (std::uint32_t x = 0; x < size.x; ++x) {
            const Color pixel = image.Pixel({x, y});
            backendImage.setPixel({x, y}, {pixel.red, pixel.green, pixel.blue, pixel.alpha});
        }
    }

    if (!backendImage.saveToFile(filePath)) {
        errorMessage = "Unable to save image: " + filePath.string();
        return false;
    }

    return true;
}

} // namespace pipeframe
