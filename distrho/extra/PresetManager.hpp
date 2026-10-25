/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DISTRHO_PRESET_MANAGER_HPP_INCLUDED
#define DISTRHO_PRESET_MANAGER_HPP_INCLUDED

#include "String.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

namespace PresetDetail {

template <typename T>
auto trySetState(T& plugin, const char* key, const char* value, int)
    -> decltype(plugin.setState(key, value), void())
{
    plugin.setState(key, value);
}

template <typename T>
void trySetState(T&, const char*, const char*, ...)
{
}

template <typename T, typename PresetType>
auto tryCaptureStates(const T& plugin, PresetType& p, int)
    -> decltype(plugin.getStateCount(), void())
{
    const uint32_t stateCount = plugin.getStateCount();
    for (uint32_t i = 0; i < stateCount; ++i)
    {
        const String& key = plugin.getStateKey(i);
        p.setState(key.buffer(), plugin.getState(key.buffer()).buffer());
    }
}

template <typename T, typename PresetType>
void tryCaptureStates(const T&, PresetType&, ...)
{
}

static inline std::string escapeJSON(const char* const str)
{
    if (str == nullptr)
        return "";

    std::string out;
    for (const char* p = str; *p != '\0'; ++p)
    {
        switch (*p)
        {
        case '\"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:   out += *p; break;
        }
    }
    return out;
}

static inline void skipWhitespace(const std::string& s, size_t& pos)
{
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos])))
        pos++;
}

static inline bool parseString(const std::string& s, size_t& pos, std::string& result)
{
    skipWhitespace(s, pos);
    if (pos >= s.size() || s[pos] != '\"')
        return false;

    pos++; // skip opening quote
    result.clear();

    while (pos < s.size())
    {
        char c = s[pos++];
        if (c == '\"')
            return true;
        if (c == '\\' && pos < s.size())
        {
            char esc = s[pos++];
            switch (esc)
            {
            case '\"': result += '\"'; break;
            case '\\': result += '\\'; break;
            case 'b':  result += '\b'; break;
            case 'f':  result += '\f'; break;
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            default:   result += esc; break;
            }
        }
        else
        {
            result += c;
        }
    }
    return false;
}

static inline bool parseNumber(const std::string& s, size_t& pos, float& num)
{
    skipWhitespace(s, pos);
    if (pos >= s.size())
        return false;

    size_t start = pos;
    if (s[pos] == '-' || s[pos] == '+') pos++;
    while (pos < s.size() && (std::isdigit(static_cast<unsigned char>(s[pos])) || s[pos] == '.' || s[pos] == 'e' || s[pos] == 'E' || s[pos] == '-' || s[pos] == '+'))
        pos++;

    if (pos > start)
    {
        std::string sub = s.substr(start, pos - start);
        char* end = nullptr;
        num = std::strtof(sub.c_str(), &end);
        return end != sub.c_str();
    }
    return false;
}

static inline bool matchChar(const std::string& s, size_t& pos, char expected)
{
    skipWhitespace(s, pos);
    if (pos < s.size() && s[pos] == expected)
    {
        pos++;
        return true;
    }
    return false;
}

} // namespace PresetDetail

// --------------------------------------------------------------------------------------------------------------------

/**
   A structured audio plugin preset containing metadata, parameter values, and state key/values.
 */
struct Preset
{
    String name;
    String category;
    String subCategory;
    String author;
    String description;
    std::vector<String> tags;
    std::vector<std::pair<String, float>> parameters;
    std::vector<std::pair<String, String>> states;

    Preset() {}

    Preset(const char* const presetName, const char* const presetCategory = "", const char* const presetAuthor = "")
        : name(presetName),
          category(presetCategory),
          author(presetAuthor)
    {}

    // ----------------------------------------------------------------------------------------------------------------
    // Parameter accessors

    void setParameter(const char* const symbol, const float value)
    {
        if (symbol == nullptr || symbol[0] == '\0')
            return;

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (parameters[i].first == symbol)
            {
                parameters[i].second = value;
                return;
            }
        }
        parameters.push_back(std::make_pair(String(symbol), value));
    }

    float getParameter(const char* const symbol, const float fallback = 0.0f) const noexcept
    {
        if (symbol == nullptr)
            return fallback;

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (parameters[i].first == symbol)
                return parameters[i].second;
        }
        return fallback;
    }

    bool hasParameter(const char* const symbol) const noexcept
    {
        if (symbol == nullptr)
            return false;

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (parameters[i].first == symbol)
                return true;
        }
        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // State accessors

    void setState(const char* const key, const char* const value)
    {
        if (key == nullptr || key[0] == '\0')
            return;

        for (size_t i = 0; i < states.size(); ++i)
        {
            if (states[i].first == key)
            {
                states[i].second = value != nullptr ? value : "";
                return;
            }
        }
        states.push_back(std::make_pair(String(key), String(value != nullptr ? value : "")));
    }

    String getState(const char* const key, const char* const fallback = "") const
    {
        if (key == nullptr)
            return String(fallback);

        for (size_t i = 0; i < states.size(); ++i)
        {
            if (states[i].first == key)
                return states[i].second;
        }
        return String(fallback);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Tag accessors

    void addTag(const char* const tag)
    {
        if (tag == nullptr || tag[0] == '\0')
            return;

        for (size_t i = 0; i < tags.size(); ++i)
        {
            if (tags[i] == tag)
                return;
        }
        tags.push_back(String(tag));
    }

    bool hasTag(const char* const tag) const noexcept
    {
        if (tag == nullptr)
            return false;

        for (size_t i = 0; i < tags.size(); ++i)
        {
            if (tags[i] == tag)
                return true;
        }
        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // JSON Serialization

    String toJSON() const
    {
        std::string out = "{\n";
        out += "  \"name\": \"" + PresetDetail::escapeJSON(name.buffer()) + "\",\n";
        out += "  \"category\": \"" + PresetDetail::escapeJSON(category.buffer()) + "\",\n";
        out += "  \"subCategory\": \"" + PresetDetail::escapeJSON(subCategory.buffer()) + "\",\n";
        out += "  \"author\": \"" + PresetDetail::escapeJSON(author.buffer()) + "\",\n";
        out += "  \"description\": \"" + PresetDetail::escapeJSON(description.buffer()) + "\",\n";

        // Tags
        out += "  \"tags\": [";
        for (size_t i = 0; i < tags.size(); ++i)
        {
            if (i > 0) out += ", ";
            out += "\"" + PresetDetail::escapeJSON(tags[i].buffer()) + "\"";
        }
        out += "],\n";

        // Parameters
        out += "  \"parameters\": {\n";
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            char numBuf[32];
            std::snprintf(numBuf, sizeof(numBuf), "%.6f", parameters[i].second);
            out += "    \"" + PresetDetail::escapeJSON(parameters[i].first.buffer()) + "\": " + numBuf;
            if (i + 1 < parameters.size())
                out += ",";
            out += "\n";
        }
        out += "  },\n";

        // States
        out += "  \"states\": {\n";
        for (size_t i = 0; i < states.size(); ++i)
        {
            out += "    \"" + PresetDetail::escapeJSON(states[i].first.buffer()) + "\": \""
                 + PresetDetail::escapeJSON(states[i].second.buffer()) + "\"";
            if (i + 1 < states.size())
                out += ",";
            out += "\n";
        }
        out += "  }\n";
        out += "}\n";

        return String(out.c_str());
    }

    bool fromJSON(const char* const jsonStr)
    {
        if (jsonStr == nullptr)
            return false;

        std::string s(jsonStr);
        size_t pos = 0;

        if (!PresetDetail::matchChar(s, pos, '{'))
            return false;

        while (pos < s.size())
        {
            PresetDetail::skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == '}')
            {
                pos++;
                return true;
            }

            std::string key;
            if (!PresetDetail::parseString(s, pos, key))
                break;

            if (!PresetDetail::matchChar(s, pos, ':'))
                break;

            if (key == "name")
            {
                std::string val;
                PresetDetail::parseString(s, pos, val);
                name = val.c_str();
            }
            else if (key == "category")
            {
                std::string val;
                PresetDetail::parseString(s, pos, val);
                category = val.c_str();
            }
            else if (key == "subCategory")
            {
                std::string val;
                PresetDetail::parseString(s, pos, val);
                subCategory = val.c_str();
            }
            else if (key == "author")
            {
                std::string val;
                PresetDetail::parseString(s, pos, val);
                author = val.c_str();
            }
            else if (key == "description")
            {
                std::string val;
                PresetDetail::parseString(s, pos, val);
                description = val.c_str();
            }
            else if (key == "tags")
            {
                tags.clear();
                if (PresetDetail::matchChar(s, pos, '['))
                {
                    while (pos < s.size())
                    {
                        PresetDetail::skipWhitespace(s, pos);
                        if (pos < s.size() && s[pos] == ']') { pos++; break; }

                        std::string tagStr;
                        if (PresetDetail::parseString(s, pos, tagStr))
                            tags.push_back(String(tagStr.c_str()));

                        PresetDetail::matchChar(s, pos, ',');
                    }
                }
            }
            else if (key == "parameters")
            {
                parameters.clear();
                if (PresetDetail::matchChar(s, pos, '{'))
                {
                    while (pos < s.size())
                    {
                        PresetDetail::skipWhitespace(s, pos);
                        if (pos < s.size() && s[pos] == '}') { pos++; break; }

                        std::string paramKey;
                        if (!PresetDetail::parseString(s, pos, paramKey)) break;
                        if (!PresetDetail::matchChar(s, pos, ':')) break;

                        float paramVal = 0.0f;
                        if (PresetDetail::parseNumber(s, pos, paramVal))
                            parameters.push_back(std::make_pair(String(paramKey.c_str()), paramVal));

                        PresetDetail::matchChar(s, pos, ',');
                    }
                }
            }
            else if (key == "states")
            {
                states.clear();
                if (PresetDetail::matchChar(s, pos, '{'))
                {
                    while (pos < s.size())
                    {
                        PresetDetail::skipWhitespace(s, pos);
                        if (pos < s.size() && s[pos] == '}') { pos++; break; }

                        std::string stateKey, stateVal;
                        if (!PresetDetail::parseString(s, pos, stateKey)) break;
                        if (!PresetDetail::matchChar(s, pos, ':')) break;
                        if (PresetDetail::parseString(s, pos, stateVal))
                            states.push_back(std::make_pair(String(stateKey.c_str()), String(stateVal.c_str())));

                        PresetDetail::matchChar(s, pos, ',');
                    }
                }
            }
            else
            {
                // Skip unknown value
                PresetDetail::skipWhitespace(s, pos);
                if (pos < s.size() && s[pos] == '\"')
                {
                    std::string dummy;
                    PresetDetail::parseString(s, pos, dummy);
                }
                else if (pos < s.size() && (std::isdigit(static_cast<unsigned char>(s[pos])) || s[pos] == '-'))
                {
                    float dummy;
                    PresetDetail::parseNumber(s, pos, dummy);
                }
            }

            PresetDetail::matchChar(s, pos, ',');
        }

        return true;
    }

    bool saveToFile(const char* const filePath) const
    {
        if (filePath == nullptr || filePath[0] == '\0')
            return false;

        std::ofstream file(filePath, std::ios::out | std::ios::trunc);
        if (!file.is_open())
            return false;

        String json = toJSON();
        file << json.buffer();
        return file.good();
    }

    bool loadFromFile(const char* const filePath)
    {
        if (filePath == nullptr || filePath[0] == '\0')
            return false;

        std::ifstream file(filePath, std::ios::in);
        if (!file.is_open())
            return false;

        std::stringstream buf;
        buf << file.rdbuf();
        return fromJSON(buf.str().c_str());
    }

    template <class PluginType>
    bool applyToPlugin(PluginType& plugin) const
    {
        const uint32_t paramCount = plugin.getParameterCount();
        for (size_t i = 0; i < parameters.size(); ++i)
        {
            const String& sym = parameters[i].first;
            const float val = parameters[i].second;

            bool found = false;
            for (uint32_t k = 0; k < paramCount; ++k)
            {
                if (plugin.getParameterSymbol(k) == sym)
                {
                    plugin.setParameterValue(k, val);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                char* end = nullptr;
                long parsedIdx = std::strtol(sym.buffer(), &end, 10);
                if (end != sym.buffer() && parsedIdx >= 0 && parsedIdx < static_cast<long>(paramCount))
                    plugin.setParameterValue(static_cast<uint32_t>(parsedIdx), val);
            }
        }

        for (size_t i = 0; i < states.size(); ++i)
        {
            PresetDetail::trySetState(plugin, states[i].first.buffer(), states[i].second.buffer(), 0);
        }

        return true;
    }
};

// --------------------------------------------------------------------------------------------------------------------

/**
   Complete hierarchical preset bank manager for DPF plugins.
 */
class PresetManager
{
public:
    PresetManager()
        : fActiveIndex(-1)
    {}

    // ----------------------------------------------------------------------------------------------------------------
    // Collection Management

    void addPreset(const Preset& preset)
    {
        fPresets.push_back(preset);
        if (fActiveIndex == -1)
            fActiveIndex = 0;
    }

    bool removePreset(const uint32_t index)
    {
        if (index >= fPresets.size())
            return false;

        fPresets.erase(fPresets.begin() + index);

        if (fPresets.empty())
            fActiveIndex = -1;
        else if (fActiveIndex >= static_cast<int32_t>(fPresets.size()))
            fActiveIndex = static_cast<int32_t>(fPresets.size()) - 1;

        return true;
    }

    void clearPresets()
    {
        fPresets.clear();
        fActiveIndex = -1;
    }

    uint32_t getPresetCount() const noexcept
    {
        return static_cast<uint32_t>(fPresets.size());
    }

    static const Preset& getFallbackPreset()
    {
        static const Preset sFallback;
        return sFallback;
    }

    const Preset& getPreset(const uint32_t index) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(index < fPresets.size(), getFallbackPreset());
        return fPresets[index];
    }

    Preset* getPresetPtr(const uint32_t index)
    {
        if (index >= fPresets.size())
            return nullptr;
        return &fPresets[index];
    }

    int32_t getActivePresetIndex() const noexcept
    {
        return fActiveIndex;
    }

    void setActivePresetIndex(const int32_t index) noexcept
    {
        if (index >= -1 && index < static_cast<int32_t>(fPresets.size()))
            fActiveIndex = index;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Categories & Search

    std::vector<String> getCategories() const
    {
        std::vector<String> cats;
        for (size_t i = 0; i < fPresets.size(); ++i)
        {
            if (fPresets[i].category.isEmpty())
                continue;

            bool exists = false;
            for (size_t c = 0; c < cats.size(); ++c)
            {
                if (cats[c] == fPresets[i].category)
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
                cats.push_back(fPresets[i].category);
        }
        return cats;
    }

    std::vector<uint32_t> findByCategory(const char* const category) const
    {
        std::vector<uint32_t> matches;
        if (category == nullptr)
            return matches;

        for (uint32_t i = 0; i < fPresets.size(); ++i)
        {
            if (fPresets[i].category == category)
                matches.push_back(i);
        }
        return matches;
    }

    std::vector<uint32_t> findByTag(const char* const tag) const
    {
        std::vector<uint32_t> matches;
        if (tag == nullptr)
            return matches;

        for (uint32_t i = 0; i < fPresets.size(); ++i)
        {
            if (fPresets[i].hasTag(tag))
                matches.push_back(i);
        }
        return matches;
    }

    std::vector<uint32_t> search(const char* const query) const
    {
        std::vector<uint32_t> matches;
        if (query == nullptr || query[0] == '\0')
            return matches;

        std::string qLower = query;
        std::transform(qLower.begin(), qLower.end(), qLower.begin(), ::tolower);

        for (uint32_t i = 0; i < fPresets.size(); ++i)
        {
            std::string nameLower = fPresets[i].name.buffer();
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

            if (nameLower.find(qLower) != std::string::npos)
            {
                matches.push_back(i);
                continue;
            }

            std::string descLower = fPresets[i].description.buffer();
            std::transform(descLower.begin(), descLower.end(), descLower.begin(), ::tolower);
            if (descLower.find(qLower) != std::string::npos)
            {
                matches.push_back(i);
                continue;
            }

            for (size_t t = 0; t < fPresets[i].tags.size(); ++t)
            {
                std::string tagLower = fPresets[i].tags[t].buffer();
                std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);
                if (tagLower.find(qLower) != std::string::npos)
                {
                    matches.push_back(i);
                    break;
                }
            }
        }
        return matches;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Plugin Interaction

    template <class PluginType>
    bool applyPreset(PluginType& plugin, const uint32_t index)
    {
        if (index >= fPresets.size())
            return false;

        if (fPresets[index].applyToPlugin(plugin))
        {
            fActiveIndex = static_cast<int32_t>(index);
            return true;
        }
        return false;
    }

    template <class PluginType>
    Preset capturePreset(const PluginType& plugin,
                         const char* const presetName,
                         const char* const presetCategory = "",
                         const char* const presetAuthor = "") const
    {
        Preset p(presetName, presetCategory, presetAuthor);

        const uint32_t paramCount = plugin.getParameterCount();
        for (uint32_t i = 0; i < paramCount; ++i)
        {
            p.setParameter(plugin.getParameterSymbol(i).buffer(), plugin.getParameterValue(i));
        }

        PresetDetail::tryCaptureStates(plugin, p, 0);

        return p;
    }

    template <class PluginType>
    bool selectNextPreset(PluginType& plugin)
    {
        if (fPresets.empty())
            return false;

        int32_t next = fActiveIndex + 1;
        if (next >= static_cast<int32_t>(fPresets.size()))
            next = 0;

        return applyPreset(plugin, static_cast<uint32_t>(next));
    }

    template <class PluginType>
    bool selectPreviousPreset(PluginType& plugin)
    {
        if (fPresets.empty())
            return false;

        int32_t prev = fActiveIndex - 1;
        if (prev < 0)
            prev = static_cast<int32_t>(fPresets.size()) - 1;

        return applyPreset(plugin, static_cast<uint32_t>(prev));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Bank JSON Serialization

    String exportBankToJSON() const
    {
        std::string out = "{\n  \"version\": 1,\n  \"presets\": [\n";
        for (size_t i = 0; i < fPresets.size(); ++i)
        {
            String pJson = fPresets[i].toJSON();
            // Indent preset JSON
            std::stringstream ss(pJson.buffer());
            std::string line;
            while (std::getline(ss, line))
                out += "    " + line + "\n";

            if (i + 1 < fPresets.size())
                out += ",\n";
        }
        out += "  ]\n}\n";
        return String(out.c_str());
    }

    bool importBankFromJSON(const char* const jsonStr)
    {
        if (jsonStr == nullptr)
            return false;

        std::string s(jsonStr);
        size_t pos = 0;

        if (!PresetDetail::matchChar(s, pos, '{'))
            return false;

        while (pos < s.size())
        {
            PresetDetail::skipWhitespace(s, pos);
            if (pos < s.size() && s[pos] == '}')
            {
                pos++;
                break;
            }

            std::string key;
            if (!PresetDetail::parseString(s, pos, key))
                break;

            if (!PresetDetail::matchChar(s, pos, ':'))
                break;

            if (key == "presets")
            {
                if (PresetDetail::matchChar(s, pos, '['))
                {
                    while (pos < s.size())
                    {
                        PresetDetail::skipWhitespace(s, pos);
                        if (pos < s.size() && s[pos] == ']') { pos++; break; }

                        if (pos < s.size() && s[pos] == '{')
                        {
                            size_t objStart = pos;
                            int braceCount = 0;
                            while (pos < s.size())
                            {
                                if (s[pos] == '{') braceCount++;
                                else if (s[pos] == '}') {
                                    braceCount--;
                                    if (braceCount == 0) { pos++; break; }
                                }
                                pos++;
                            }
                            std::string objStr = s.substr(objStart, pos - objStart);
                            Preset p;
                            if (p.fromJSON(objStr.c_str()))
                                addPreset(p);
                        }

                        PresetDetail::matchChar(s, pos, ',');
                    }
                }
            }
            else
            {
                // skip
                float dummy;
                PresetDetail::parseNumber(s, pos, dummy);
            }

            PresetDetail::matchChar(s, pos, ',');
        }

        return true;
    }

    bool saveBankToFile(const char* const filePath) const
    {
        if (filePath == nullptr || filePath[0] == '\0')
            return false;

        std::ofstream file(filePath, std::ios::out | std::ios::trunc);
        if (!file.is_open())
            return false;

        String json = exportBankToJSON();
        file << json.buffer();
        return file.good();
    }

    bool loadBankFromFile(const char* const filePath)
    {
        if (filePath == nullptr || filePath[0] == '\0')
            return false;

        std::ifstream file(filePath, std::ios::in);
        if (!file.is_open())
            return false;

        std::stringstream buf;
        buf << file.rdbuf();
        return importBankFromJSON(buf.str().c_str());
    }

private:
    std::vector<Preset> fPresets;
    int32_t fActiveIndex;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PRESET_MANAGER_HPP_INCLUDED
