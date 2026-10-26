#pragma once

#include <charconv>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace RoverLab::reports::detail {
// Bounded JSON reader for the versioned RL9 journal. No evaluation or external dependencies.
struct Json {
  using Array = std::vector<Json>;
  using Object = std::map<std::string, Json>;
  std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;

  const Json& At(const std::string& key) const {
    const auto* object = std::get_if<Object>(&value);
    if (!object || !object->contains(key)) throw std::runtime_error("Missing field: " + key);
    return object->at(key);
  }
  bool Has(const std::string& key) const {
    const auto* object = std::get_if<Object>(&value);
    return object && object->contains(key);
  }
  const std::string& Text() const { return std::get<std::string>(value); }
  double Number() const { return std::get<double>(value); }
  bool Boolean() const { return std::get<bool>(value); }
  const Array& Items() const { return std::get<Array>(value); }
};

class JsonReader {
public:
  explicit JsonReader(std::string_view source) : source(source) {}

  Json Read() {
    auto result = Value(0);
    Space();
    Need(position == source.size());
    return result;
  }

private:
  void Need(bool okay) const {
    if (!okay) throw std::runtime_error("Invalid JSON near byte " + std::to_string(position));
  }
  void Space() {
    while (position < source.size() && (source[position] == ' ' || source[position] == '\t' ||
                                         source[position] == '\r' || source[position] == '\n'))
      ++position;
  }
  bool Take(char c) {
    Space();
    if (position < source.size() && source[position] == c) {
      ++position;
      return true;
    }
    return false;
  }
  unsigned Hex() {
    unsigned value = 0;
    for (int i = 0; i < 4; ++i) {
      Need(position < source.size());
      const char c = source[position++];
      const int n = c >= '0' && c <= '9'   ? c - '0'
                    : c >= 'a' && c <= 'f' ? c - 'a' + 10
                    : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                           : -1;
      Need(n >= 0);
      value = value * 16 + unsigned(n);
    }
    return value;
  }
  void Utf8(std::string& out, unsigned cp) {
    if (cp < 0x80)
      out += char(cp);
    else if (cp < 0x800) {
      out += char(0xc0 | (cp >> 6));
      out += char(0x80 | (cp & 63));
    } else if (cp < 0x10000) {
      out += char(0xe0 | (cp >> 12));
      out += char(0x80 | ((cp >> 6) & 63));
      out += char(0x80 | (cp & 63));
    } else {
      out += char(0xf0 | (cp >> 18));
      out += char(0x80 | ((cp >> 12) & 63));
      out += char(0x80 | ((cp >> 6) & 63));
      out += char(0x80 | (cp & 63));
    }
  }
  std::string String() {
    Need(Take('"'));
    std::string out;
    while (position < source.size()) {
      unsigned char c = source[position++];
      if (c == '"') return out;
      Need(c >= 32);
      if (c != '\\') {
        out += char(c);
        continue;
      }
      Need(position < source.size());
      switch (source[position++]) {
        case '"':
          out += '"';
          break;
        case '\\':
          out += '\\';
          break;
        case '/':
          out += '/';
          break;
        case 'b':
          out += '\b';
          break;
        case 'f':
          out += '\f';
          break;
        case 'n':
          out += '\n';
          break;
        case 'r':
          out += '\r';
          break;
        case 't':
          out += '\t';
          break;
        case 'u': {
          unsigned cp = Hex();
          if (cp >= 0xd800 && cp <= 0xdbff) {
            Need(position + 2 <= source.size() && source.substr(position, 2) == "\\u");
            position += 2;
            const unsigned low = Hex();
            Need(low >= 0xdc00 && low <= 0xdfff);
            cp = 0x10000 + ((cp - 0xd800) << 10) + low - 0xdc00;
          } else
            Need(cp < 0xdc00 || cp > 0xdfff);
          Utf8(out, cp);
          break;
        }
        default:
          Need(false);
      }
    }
    Need(false);
    return {};
  }
  Json Value(unsigned depth) {
    Need(depth <= 32);
    Space();
    Need(position < source.size());
    if (source[position] == '"') return {String()};
    if (Take('{')) {
      Json::Object object;
      if (Take('}')) return {object};
      do {
        auto key = String();
        Need(Take(':'));
        auto item = Value(depth + 1);
        Need(object.emplace(std::move(key), std::move(item)).second);
        if (Take('}')) return {object};
      } while (Take(','));
      Need(false);
    }
    if (Take('[')) {
      Json::Array array;
      if (Take(']')) return {array};
      do {
        array.push_back(Value(depth + 1));
        if (Take(']')) return {array};
      } while (Take(','));
      Need(false);
    }
    for (auto [word, value] : std::vector<std::pair<std::string_view, Json>>{
           {"true", Json{true}}, {"false", Json{false}}, {"null", Json{nullptr}}}) {
      if (source.substr(position, word.size()) == word) {
        position += word.size();
        return value;
      }
    }
    const auto start = position;
    if (source[position] == '-') ++position;
    Need(position < source.size());
    const auto digit = [&] {
      return position < source.size() && source[position] >= '0' && source[position] <= '9';
    };
    if (source[position] == '0')
      ++position;
    else {
      Need(digit() && source[position] != '0');
      while (digit())
        ++position;
    }
    if (position < source.size() && source[position] == '.') {
      ++position;
      Need(digit());
      while (digit())
        ++position;
    }
    if (position < source.size() && (source[position] == 'e' || source[position] == 'E')) {
      ++position;
      if (position < source.size() && (source[position] == '+' || source[position] == '-'))
        ++position;
      Need(digit());
      while (digit())
        ++position;
    }
    double number{};
    const auto result = std::from_chars(source.data() + start, source.data() + position, number);
    Need(
      result.ec == std::errc{} && result.ptr == source.data() + position && std::isfinite(number));
    return {number};
  }
  std::string_view source;
  std::size_t position{};
};
}  // namespace RoverLab::reports::detail
