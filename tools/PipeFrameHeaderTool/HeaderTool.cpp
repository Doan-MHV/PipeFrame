#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{
struct PropertyInfo
{
    bool isEnum = false;
    bool isSeconds = false;
    std::string displayName;
    std::string visibility = "Edit";
    std::string storage = "Save";
    std::string minValue;
    std::string maxValue;
    std::string stepValue;
    std::vector<std::string> enumOptions;
    std::string fieldType;
    std::string fieldName;
    std::string jsonName;
    bool hasJsonNameOverride = false;
};

struct ComponentInfo
{
    std::string includePath;
    std::string typeName;
    std::string displayName;
    std::string jsonTypeName;
    bool isEngineComponent = false;
    bool editorAddable = true;
    bool editorRemovable = true;
    bool editorInspectable = true;
    bool metadataSerializable = true;
    bool manualDescriptor = false;
    std::vector<PropertyInfo> properties;
};

std::string ReadTextFile(const std::filesystem::path& path)
{
    std::ifstream file(path);
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

bool WriteTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());

    std::ifstream existing(path);
    if (existing)
    {
        std::ostringstream current;
        current << existing.rdbuf();
        if (current.str() == text)
        {
            return true;
        }
    }

    std::ofstream file(path);
    if (!file)
    {
        return false;
    }

    file << text;
    return true;
}

std::string Trim(std::string value)
{
    auto isSpace = [](unsigned char character)
    {
        return std::isspace(character) != 0;
    };

    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char character)
    {
        return !isSpace(static_cast<unsigned char>(character));
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](char character)
    {
        return !isSpace(static_cast<unsigned char>(character));
    }).base(), value.end());
    return value;
}

std::string StripQuotes(std::string value)
{
    value = Trim(std::move(value));
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

bool IsQuotedString(const std::string& value)
{
    const std::string trimmed = Trim(value);
    return trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"';
}

std::string RemoveComments(const std::string& text)
{
    std::string result;
    result.reserve(text.size());

    bool inString = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    for (size_t index = 0; index < text.size(); ++index)
    {
        const char character = text[index];
        const char nextCharacter = index + 1 < text.size() ? text[index + 1] : '\0';

        if (inLineComment)
        {
            if (character == '\n')
            {
                inLineComment = false;
                result.push_back(character);
            }
            continue;
        }

        if (inBlockComment)
        {
            if (character == '*' && nextCharacter == '/')
            {
                inBlockComment = false;
                ++index;
            }
            else if (character == '\n')
            {
                result.push_back(character);
            }
            continue;
        }

        if (!inString && character == '/' && nextCharacter == '/')
        {
            inLineComment = true;
            ++index;
            continue;
        }

        if (!inString && character == '/' && nextCharacter == '*')
        {
            inBlockComment = true;
            ++index;
            continue;
        }

        if (character == '"' && (index == 0 || text[index - 1] != '\\'))
        {
            inString = !inString;
        }

        result.push_back(character);
    }

    return result;
}

std::string StripNamespace(std::string value)
{
    value = Trim(std::move(value));
    if (value.starts_with("PF::"))
    {
        return value.substr(4);
    }
    return value;
}

std::string StripFunctionArgument(const std::string& value, const std::string& functionName)
{
    std::string trimmed = Trim(value);
    const std::string prefix = "PF::" + functionName + "(";
    const std::string legacyPrefix = functionName + "(";

    size_t openParen = std::string::npos;
    if (trimmed.starts_with(prefix))
    {
        openParen = prefix.size() - 1;
    }
    else if (trimmed.starts_with(legacyPrefix))
    {
        openParen = legacyPrefix.size() - 1;
    }

    if (openParen == std::string::npos || trimmed.back() != ')')
    {
        return {};
    }

    return StripQuotes(trimmed.substr(openParen + 1, trimmed.size() - openParen - 2));
}

bool IsVisibilityToken(const std::string& value)
{
    const std::string token = StripNamespace(value);
    return token == "Edit" || token == "ReadOnly" || token == "Hidden";
}

bool IsStorageToken(const std::string& value)
{
    const std::string token = StripNamespace(value);
    return token == "Save" || token == "RuntimeOnly";
}

bool IsComponentFlagToken(const std::string& value)
{
    const std::string token = StripNamespace(value);
    return
        token == "Engine" ||
        token == "NotAddable" ||
        token == "NotRemovable" ||
        token == "Hidden" ||
        token == "NotSerializable" ||
        token == "Manual";
}

std::string RemoveComponentSuffix(std::string value)
{
    constexpr std::string_view suffix = "Component";
    if (value.size() >= suffix.size() && value.ends_with(suffix))
    {
        value.resize(value.size() - suffix.size());
    }
    return value;
}

std::string RemoveBooleanPrefix(std::string value)
{
    if (
        value.size() > 2 &&
        value[0] == 'i' &&
        value[1] == 's' &&
        std::isupper(static_cast<unsigned char>(value[2])) != 0
    )
    {
        value = value.substr(2);
        value[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[0])));
    }
    return value;
}

std::string ToSnakeCase(std::string value)
{
    value = RemoveBooleanPrefix(std::move(value));

    std::string result;
    for (size_t index = 0; index < value.size(); ++index)
    {
        const char character = value[index];
        if (std::isupper(static_cast<unsigned char>(character)) != 0)
        {
            if (index > 0 && !result.empty() && result.back() != '_')
            {
                result.push_back('_');
            }
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
        }
        else
        {
            result.push_back(character);
        }
    }
    return result;
}

std::string ToDisplayName(std::string value)
{
    value = RemoveBooleanPrefix(RemoveComponentSuffix(std::move(value)));

    std::string result;
    for (size_t index = 0; index < value.size(); ++index)
    {
        const char character = value[index];
        if (index > 0 && std::isupper(static_cast<unsigned char>(character)) != 0)
        {
            result.push_back(' ');
        }

        if (index == 0)
        {
            result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(character))));
        }
        else
        {
            result.push_back(character);
        }
    }
    return result;
}

std::vector<std::string> SplitArguments(const std::string& arguments)
{
    std::vector<std::string> result;
    std::string current;
    bool inString = false;
    int parenDepth = 0;

    for (size_t index = 0; index < arguments.size(); ++index)
    {
        const char character = arguments[index];
        if (character == '"' && (index == 0 || arguments[index - 1] != '\\'))
        {
            inString = !inString;
        }
        else if (!inString && character == '(')
        {
            ++parenDepth;
        }
        else if (!inString && character == ')')
        {
            --parenDepth;
        }

        if (!inString && parenDepth == 0 && character == ',')
        {
            result.push_back(Trim(current));
            current.clear();
            continue;
        }

        current.push_back(character);
    }

    if (!current.empty())
    {
        result.push_back(Trim(current));
    }

    return result;
}

std::optional<size_t> FindMatchingParen(const std::string& text, size_t openParen)
{
    bool inString = false;
    int depth = 0;

    for (size_t index = openParen; index < text.size(); ++index)
    {
        const char character = text[index];
        if (character == '"' && (index == 0 || text[index - 1] != '\\'))
        {
            inString = !inString;
            continue;
        }

        if (inString)
        {
            continue;
        }

        if (character == '(')
        {
            ++depth;
        }
        else if (character == ')')
        {
            --depth;
            if (depth == 0)
            {
                return index;
            }
        }
    }

    return std::nullopt;
}

std::optional<size_t> FindMatchingBrace(const std::string& text, size_t openBrace)
{
    bool inString = false;
    int depth = 0;

    for (size_t index = openBrace; index < text.size(); ++index)
    {
        const char character = text[index];
        if (character == '"' && (index == 0 || text[index - 1] != '\\'))
        {
            inString = !inString;
            continue;
        }

        if (inString)
        {
            continue;
        }

        if (character == '{')
        {
            ++depth;
        }
        else if (character == '}')
        {
            --depth;
            if (depth == 0)
            {
                return index;
            }
        }
    }

    return std::nullopt;
}

std::string ParseIdentifier(const std::string& text, size_t& cursor)
{
    while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor])) != 0)
    {
        ++cursor;
    }

    const size_t begin = cursor;
    while (cursor < text.size())
    {
        const char character = text[cursor];
        if (!(std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '_'))
        {
            break;
        }
        ++cursor;
    }

    return text.substr(begin, cursor - begin);
}

std::optional<PropertyInfo> ParseProperty(
    const std::string& body,
    size_t propertyStart,
    bool isEnum,
    bool isSeconds
)
{
    const size_t openParen = body.find('(', propertyStart);
    if (openParen == std::string::npos)
    {
        return std::nullopt;
    }

    const std::optional<size_t> closeParen = FindMatchingParen(body, openParen);
    if (!closeParen.has_value())
    {
        return std::nullopt;
    }

    const std::vector<std::string> args =
        SplitArguments(body.substr(openParen + 1, *closeParen - openParen - 1));
    if (args.empty())
    {
        return std::nullopt;
    }

    const size_t semicolon = body.find(';', *closeParen + 1);
    if (semicolon == std::string::npos)
    {
        return std::nullopt;
    }

    std::string declaration = Trim(body.substr(*closeParen + 1, semicolon - *closeParen - 1));
    const size_t initializer = declaration.find('=');
    if (initializer != std::string::npos)
    {
        declaration = Trim(declaration.substr(0, initializer));
    }

    const size_t nameEnd = declaration.find_last_not_of(" \t\r\n");
    if (nameEnd == std::string::npos)
    {
        return std::nullopt;
    }

    size_t nameBegin = nameEnd;
    while (nameBegin > 0)
    {
        const char character = declaration[nameBegin - 1];
        if (!(std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '_'))
        {
            break;
        }
        --nameBegin;
    }

    PropertyInfo property;
    property.fieldName = declaration.substr(nameBegin, nameEnd - nameBegin + 1);
    property.displayName = ToDisplayName(property.fieldName);
    property.fieldType = Trim(declaration.substr(0, nameBegin));
    property.isEnum = isEnum;
    property.isSeconds = isSeconds;

    std::vector<std::string> numericArgs;
    bool oldStyleDisplayNameConsumed = false;

    if (!args.empty() && IsQuotedString(args[0]))
    {
        property.displayName = StripQuotes(args[0]);
        oldStyleDisplayNameConsumed = true;
    }

    for (size_t argIndex = oldStyleDisplayNameConsumed ? 1 : 0; argIndex < args.size(); ++argIndex)
    {
        const std::string arg = Trim(args[argIndex]);
        if (arg.empty())
        {
            continue;
        }

        if (const std::string displayName = StripFunctionArgument(arg, "DisplayName"); !displayName.empty())
        {
            property.displayName = displayName;
            continue;
        }

        if (const std::string jsonName = StripFunctionArgument(arg, "JsonName"); !jsonName.empty())
        {
            property.jsonName = jsonName;
            property.hasJsonNameOverride = true;
            continue;
        }

        if (IsVisibilityToken(arg))
        {
            property.visibility = StripNamespace(arg);
            continue;
        }

        if (IsStorageToken(arg))
        {
            property.storage = StripNamespace(arg);
            continue;
        }

        if (isEnum && IsQuotedString(arg))
        {
            property.enumOptions.push_back(StripQuotes(arg));
            continue;
        }

        if (!isEnum && IsQuotedString(arg))
        {
            property.jsonName = StripQuotes(arg);
            property.hasJsonNameOverride = true;
            continue;
        }

        if (!isEnum)
        {
            numericArgs.push_back(arg);
        }
    }

    if (numericArgs.size() >= 3)
    {
        property.minValue = numericArgs[0];
        property.maxValue = numericArgs[1];
        property.stepValue = numericArgs[2];
    }

    return property;
}

std::vector<PropertyInfo> ParseProperties(const std::string& body)
{
    std::vector<PropertyInfo> properties;
    size_t cursor = 0;

    while (true)
    {
        const size_t regularPropertyStart = body.find("PF_PROPERTY", cursor);
        const size_t enumPropertyStart = body.find("PF_ENUM_PROPERTY", cursor);
        const size_t secondsPropertyStart = body.find("PF_SECONDS_PROPERTY", cursor);
        if (
            regularPropertyStart == std::string::npos &&
            enumPropertyStart == std::string::npos &&
            secondsPropertyStart == std::string::npos
        )
        {
            break;
        }

        size_t propertyStart = std::string::npos;
        bool useEnum = false;
        bool useSeconds = false;

        auto chooseProperty = [&](size_t candidateStart, bool candidateIsEnum, bool candidateIsSeconds)
        {
            if (candidateStart == std::string::npos)
            {
                return;
            }

            if (propertyStart == std::string::npos || candidateStart < propertyStart)
            {
                propertyStart = candidateStart;
                useEnum = candidateIsEnum;
                useSeconds = candidateIsSeconds;
            }
        };

        chooseProperty(regularPropertyStart, false, false);
        chooseProperty(enumPropertyStart, true, false);
        chooseProperty(secondsPropertyStart, false, true);

        if (std::optional<PropertyInfo> property = ParseProperty(body, propertyStart, useEnum, useSeconds))
        {
            properties.push_back(*property);
        }

        cursor = propertyStart + 1;
    }

    return properties;
}

std::vector<ComponentInfo> ParseComponents(
    const std::string& text,
    const std::string& includePath
)
{
    std::vector<ComponentInfo> components;
    size_t cursor = 0;

    while (true)
    {
        const size_t componentStart = text.find("PF_COMPONENT", cursor);
        if (componentStart == std::string::npos)
        {
            break;
        }

        const size_t openParen = text.find('(', componentStart);
        if (openParen == std::string::npos)
        {
            break;
        }

        const std::optional<size_t> closeParen = FindMatchingParen(text, openParen);
        if (!closeParen.has_value())
        {
            break;
        }

        const std::vector<std::string> args =
            SplitArguments(text.substr(openParen + 1, *closeParen - openParen - 1));

        const size_t structKeyword = text.find("struct", *closeParen + 1);
        if (structKeyword == std::string::npos)
        {
            cursor = *closeParen + 1;
            continue;
        }

        size_t nameCursor = structKeyword + std::string("struct").size();
        const std::string componentName = ParseIdentifier(text, nameCursor);
        if (componentName.empty())
        {
            cursor = nameCursor;
            continue;
        }

        const size_t openBrace = text.find('{', nameCursor);
        if (openBrace == std::string::npos)
        {
            cursor = nameCursor;
            continue;
        }

        const std::optional<size_t> closeBrace = FindMatchingBrace(text, openBrace);
        if (!closeBrace.has_value())
        {
            cursor = openBrace + 1;
            continue;
        }

        ComponentInfo component;
        component.includePath = includePath;
        component.typeName = componentName;
        component.displayName = ToDisplayName(componentName);

        bool hasJsonNameOverride = false;
        bool consumedOldDisplayName = false;

        if (!args.empty() && IsQuotedString(args[0]))
        {
            component.displayName = StripQuotes(args[0]);
            consumedOldDisplayName = true;
        }

        if (args.size() >= 2 && IsQuotedString(args[1]))
        {
            component.jsonTypeName = StripQuotes(args[1]);
            hasJsonNameOverride = true;
        }

        for (size_t argIndex = consumedOldDisplayName ? 1 : 0; argIndex < args.size(); ++argIndex)
        {
            const std::string arg = Trim(args[argIndex]);
            const std::string flag = StripNamespace(arg);

            if (IsQuotedString(arg))
            {
                continue;
            }

            if (const std::string displayName = StripFunctionArgument(arg, "DisplayName"); !displayName.empty())
            {
                component.displayName = displayName;
                continue;
            }

            if (const std::string jsonName = StripFunctionArgument(arg, "JsonName"); !jsonName.empty())
            {
                component.jsonTypeName = jsonName;
                hasJsonNameOverride = true;
                continue;
            }

            if (flag == "Engine")
            {
                component.isEngineComponent = true;
            }
            else if (flag == "NotAddable")
            {
                component.editorAddable = false;
            }
            else if (flag == "NotRemovable")
            {
                component.editorRemovable = false;
            }
            else if (flag == "Hidden")
            {
                component.editorInspectable = false;
            }
            else if (flag == "NotSerializable")
            {
                component.metadataSerializable = false;
            }
            else if (flag == "Manual")
            {
                component.manualDescriptor = true;
            }
        }

        if (!hasJsonNameOverride)
        {
            component.jsonTypeName = component.isEngineComponent
                ? ToSnakeCase(RemoveComponentSuffix(componentName))
                : componentName;
        }
        component.properties = ParseProperties(text.substr(openBrace + 1, *closeBrace - openBrace - 1));
        for (PropertyInfo& property : component.properties)
        {
            if (!property.hasJsonNameOverride)
            {
                property.jsonName = component.isEngineComponent
                    ? ToSnakeCase(property.fieldName)
                    : property.fieldName;
            }
        }
        if (!component.manualDescriptor)
        {
            components.push_back(component);
        }

        cursor = *closeBrace + 1;
    }

    return components;
}

std::string ToGeneratedVisibility(const std::string& value)
{
    const std::string token = StripNamespace(value);
    if (token == "ReadOnly")
    {
        return "PropertyVisibility::ReadOnly";
    }
    if (token == "Hidden")
    {
        return "PropertyVisibility::Hidden";
    }
    return "PropertyVisibility::Edit";
}

std::string ToGeneratedStorage(const std::string& value)
{
    const std::string token = StripNamespace(value);
    if (token == "RuntimeOnly")
    {
        return "PropertyStorage::RuntimeOnly";
    }
    return "PropertyStorage::Save";
}

std::string ToFactoryName(const std::string& type)
{
    if (type == "int")
    {
        return "IntProperty";
    }
    if (type == "float")
    {
        return "FloatProperty";
    }
    if (type == "double")
    {
        return "DoubleProperty";
    }
    if (type == "bool")
    {
        return "BoolProperty";
    }
    if (type == "std::string")
    {
        return "StringProperty";
    }
    if (type == "glm::vec2")
    {
        return "Vec2Property";
    }
    if (type == "SDL_FRect")
    {
        return "SplitRectProperty";
    }
    if (type == "SDL_Color")
    {
        return "ColorProperty";
    }
    if (type == "nlohmann::json")
    {
        return "RootJsonObjectProperty";
    }

    return {};
}

std::string BuildProperty(const ComponentInfo& component, const PropertyInfo& property)
{
    if (property.isEnum)
    {
        std::ostringstream output;
        output << "            EnumProperty<" << component.typeName << ", " << property.fieldType << ">(\n"
               << "                \"" << property.jsonName << "\",\n"
               << "                \"" << property.displayName << "\",\n"
               << "                &" << component.typeName << "::" << property.fieldName << ",\n"
               << "                {";

        for (size_t index = 0; index < property.enumOptions.size(); ++index)
        {
            if (index > 0)
            {
                output << ", ";
            }
            output << "\"" << property.enumOptions[index] << "\"";
        }

        output << "},\n"
               << "                " << ToGeneratedVisibility(property.visibility) << ",\n"
               << "                " << ToGeneratedStorage(property.storage) << "\n"
               << "            )";
        return output.str();
    }

    const std::string factoryName =
        property.isSeconds ? "MillisecondsAsSecondsProperty" : ToFactoryName(property.fieldType);
    if (factoryName.empty())
    {
        return {};
    }

    std::ostringstream output;
    output << "            " << factoryName << "<" << component.typeName << ">(\n"
           << "                \"" << property.jsonName << "\",\n"
           << "                \"" << property.displayName << "\",\n"
           << "                &" << component.typeName << "::" << property.fieldName;

    if (
        factoryName == "BoolProperty" ||
        factoryName == "StringProperty" ||
        factoryName == "ColorProperty" ||
        factoryName == "RootJsonObjectProperty"
    )
    {
        output << ",\n"
               << "                " << ToGeneratedVisibility(property.visibility) << ",\n"
               << "                " << ToGeneratedStorage(property.storage) << "\n"
               << "            )";
        return output.str();
    }

    const std::string minValue = property.minValue.empty() ? "0" : property.minValue;
    const std::string maxValue = property.maxValue.empty() ? "0" : property.maxValue;
    const std::string stepValue = property.stepValue.empty() ? "1" : property.stepValue;

    output << ",\n"
           << "                " << minValue << ",\n"
           << "                " << maxValue << ",\n"
           << "                " << stepValue << ",\n"
           << "                " << ToGeneratedVisibility(property.visibility) << ",\n"
           << "                " << ToGeneratedStorage(property.storage) << "\n"
           << "            )";
    return output.str();
}

std::string BuildGeneratedHeader(
    const std::vector<ComponentInfo>& components,
    const std::string& registerFunctionName
)
{
    std::ostringstream output;
    output << "#ifndef PIPEFRAME_GENERATED_PROJECT_COMPONENTS_H\n"
           << "#define PIPEFRAME_GENERATED_PROJECT_COMPONENTS_H\n\n"
           << "#include \"Reflection/ComponentDescriptor.h\"\n\n";

    for (const ComponentInfo& component : components)
    {
        output << "#include \"" << component.includePath << "\"\n";
    }

    output << "\n";

    for (const ComponentInfo& component : components)
    {
        output << "class " << component.typeName << "Descriptor final : public "
               << (component.isEngineComponent ? "EngineComponentDescriptor<" : "ComponentDescriptor<")
               << component.typeName << ">\n"
               << "{\n"
               << "protected:\n"
               << "    std::string GetTypeName() const override { return \"" << component.jsonTypeName << "\"; }\n"
               << "    std::string GetDisplayName() const override { return \"" << component.displayName << "\"; }\n";

        if (!component.editorAddable)
        {
            output << "    bool IsEditorAddable() const override { return false; }\n";
        }
        if (!component.editorRemovable)
        {
            output << "    bool IsEditorRemovable() const override { return false; }\n";
        }
        if (!component.editorInspectable)
        {
            output << "    bool IsEditorInspectable() const override { return false; }\n";
        }
        if (!component.metadataSerializable)
        {
            output << "    bool IsSerializable() const override { return false; }\n";
        }

        output << "\n"
               << "    std::vector<PropertyMetadata> GetProperties() const override\n"
               << "    {\n"
               << "        return {\n";

        bool wroteProperty = false;
        for (const PropertyInfo& property : component.properties)
        {
            const std::string generatedProperty = BuildProperty(component, property);
            if (generatedProperty.empty())
            {
                continue;
            }

            if (wroteProperty)
            {
                output << ",\n";
            }
            output << generatedProperty;
            wroteProperty = true;
        }

        output << "\n"
               << "        };\n"
               << "    }\n\n"
               << "    nlohmann::json SerializeComponent(Entity entity) const override\n"
               << "    {\n"
               << "        return SerializeProperties(entity);\n"
               << "    }\n\n"
               << "    void ApplyComponent(Entity entity, const nlohmann::json& json) const override\n"
               << "    {\n"
               << "        ApplyProperties(entity, json);\n"
               << "    }\n"
               << "};\n\n";
    }

    output << "inline void " << registerFunctionName << "(ComponentRegistry& registry)\n"
           << "{\n";
    for (const ComponentInfo& component : components)
    {
        output << "    RegisterComponentDescriptor<" << component.typeName << "Descriptor>(registry);\n";
    }
    output << "}\n\n"
           << "#endif // PIPEFRAME_GENERATED_PROJECT_COMPONENTS_H\n";
    return output.str();
}
}

int main(int argc, char** argv)
{
    if (argc != 3 && argc != 4)
    {
        std::cerr << "Usage: PipeFrameHeaderTool <source-root> <output-header> [register-function]\n";
        return 1;
    }

    const std::filesystem::path sourceRoot = std::filesystem::absolute(argv[1]);
    const std::filesystem::path outputHeader = std::filesystem::absolute(argv[2]);
    const std::string registerFunctionName =
        argc == 4 ? argv[3] : "RegisterGeneratedProjectComponents";
    std::vector<ComponentInfo> components;

    for (const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator(sourceRoot))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".h")
        {
            continue;
        }

        if (entry.path().string().find("/Generated/") != std::string::npos)
        {
            continue;
        }

        const std::filesystem::path relativePath = std::filesystem::relative(entry.path(), sourceRoot);
        const std::string includePath = relativePath.generic_string();
        const std::string text = RemoveComments(ReadTextFile(entry.path()));
        std::vector<ComponentInfo> fileComponents = ParseComponents(text, includePath);
        components.insert(components.end(), fileComponents.begin(), fileComponents.end());
    }

    std::sort(components.begin(), components.end(), [](const ComponentInfo& left, const ComponentInfo& right)
    {
        return left.typeName < right.typeName;
    });

    if (!WriteTextFile(outputHeader, BuildGeneratedHeader(components, registerFunctionName)))
    {
        std::cerr << "Failed to write generated header: " << outputHeader << "\n";
        return 1;
    }

    std::cout << "Generated " << components.size() << " component descriptor(s): "
              << outputHeader << "\n";
    return 0;
}
