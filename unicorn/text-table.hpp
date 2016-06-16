#pragma once

#include "unicorn/core.hpp"
#include "unicorn/format.hpp"
#include "unicorn/string.hpp"
#include <ostream>
#include <string>
#include <vector>

namespace Unicorn {

    constexpr Kwarg<bool> tab_unfill;
    constexpr Kwarg<uint32_t> tab_flags;
    constexpr Kwarg<size_t> tab_margin, tab_spacing;
    constexpr Kwarg<u8string> tab_ditto, tab_empty;

    class TextTable {
    public:
        TextTable(): formats(), cells(1) {}
        template <typename T> TextTable& operator<<(const T& t) { add_cell(t); return *this; }
        TextTable& operator<<(char t) { character_code(t); return *this; }
        TextTable& operator<<(char16_t t) { add_str(str_char(t)); return *this; }
        TextTable& operator<<(char32_t t) { add_str(str_char(t)); return *this; }
        TextTable& operator<<(wchar_t t) { add_str(str_char(t)); return *this; }
        TextTable& operator<<(char* t) { add_str(cstr(t)); return *this; }
        TextTable& operator<<(char16_t* t) { add_str(to_utf8(cstr(t))); return *this; }
        TextTable& operator<<(char32_t* t) { add_str(to_utf8(cstr(t))); return *this; }
        TextTable& operator<<(wchar_t* t) { add_str(to_utf8(cstr(t))); return *this; }
        TextTable& operator<<(const char* t) { add_str(cstr(t)); return *this; }
        TextTable& operator<<(const char16_t* t) { add_str(to_utf8(cstr(t))); return *this; }
        TextTable& operator<<(const char32_t* t) { add_str(to_utf8(cstr(t))); return *this; }
        TextTable& operator<<(const wchar_t* t) { add_str(to_utf8(cstr(t))); return *this; }
        TextTable& operator<<(const u8string& t) { add_str(t); return *this; }
        TextTable& operator<<(const u16string& t) { add_str(to_utf8(t)); return *this; }
        TextTable& operator<<(const u32string& t) { add_str(to_utf8(t)); return *this; }
        TextTable& operator<<(const wstring& t) { add_str(to_utf8(t)); return *this; }
        void clear() noexcept { cells.clear(); cells.resize(1); formats.clear(); }
        template <typename... FS> void format(const u8string& f, const FS&... fs) { format(f); format(fs...); }
        void format(const u8string& f) { formats.push_back(Format(f)); }
        template <typename... Args> u8string str(const Args&... args) const;
        template <typename... Args> void write(std::ostream& out, const Args&... args) const;
    private:
        struct layout_spec {
            uint32_t flags = grapheme_units;
            u8string ditto = "''";
            u8string empty = "--";
            size_t margin = 0;
            size_t spacing = 2;
            bool unfill = false;
        };
        vector<Format> formats;
        vector<vector<u8string>> cells;
        template <typename T> void add_cell(const T& t);
        void add_str(const u8string& t);
        void character_code(char c);
        void force_break();
        template <typename... Args> static layout_spec parse_args(const Args&... args);
        void write_table(const layout_spec& spec, vector<u8string>& lines) const;
    };

    template <typename... Args>
    u8string TextTable::str(const Args&... args) const {
        auto spec = parse_args(args...);
        vector<u8string> lines;
        write_table(spec, lines);
        auto result = join(lines, "\n");
        if (! lines.empty())
            result += '\n';
        return result;
    }

    template <typename... Args>
    void TextTable::write(std::ostream& out, const Args&... args) const {
        auto spec = parse_args(args...);
        vector<u8string> lines;
        write_table(spec, lines);
        for (auto& line: lines)
            out << line << '\n';
    }

    template <typename T>
    void TextTable::add_cell(const T& t) {
        size_t index = cells.back().size();
        u8string cell;
        if (index < formats.size() && ! formats[index].format().empty())
            cell = formats[index](t);
        else
            cell = to_str(t);
        cells.back().push_back(str_trim(cell));
    }

    template <typename... Args>
    TextTable::layout_spec TextTable::parse_args(const Args&... args) {
        layout_spec spec;
        kwget(tab_flags, spec.flags, args...);
        kwget(tab_ditto, spec.ditto, args...);
        kwget(tab_empty, spec.empty, args...);
        kwget(tab_margin, spec.margin, args...);
        kwget(tab_spacing, spec.spacing, args...);
        kwget(tab_unfill, spec.unfill, args...);
        return spec;
    }

    inline std::ostream& operator<<(std::ostream& out, const TextTable& tab) {
        tab.write(out);
        return out;
    }

}