#include <exception>
#include <meta>
#include <print>
#include <ranges>
#include <stdexcept>
#include <utility>

constexpr std::size_t FIXED_STRING_SIZE = 32u;
constexpr std::size_t MAX_BLOCK_FIELDS = 16u;

struct consteval_error : std::exception
{
    char buf[160] = {};

    constexpr explicit consteval_error(const char *m) noexcept
    {
        std::size_t i = 0;
        for (; m[i] && i + 1 < sizeof(buf); ++i)
            buf[i] = m[i];
        buf[i] = '\0';
    }

    constexpr consteval_error(const char *m, std::string_view value) noexcept
    {
        std::size_t i = 0;
        for (; m[i] && i + 1 < sizeof(buf); ++i)
            buf[i] = m[i];
        if (i + 2 < sizeof(buf))
        {
            buf[i++] = ':';
            buf[i++] = ' ';
        }
        for (std::size_t j = 0; j < value.size() && i + 1 < sizeof(buf); ++j)
            buf[i++] = value[j];
        buf[i] = '\0';
    }

    constexpr const char *what() const noexcept override
    {
        return buf;
    }
};

template <unsigned long Size> struct FixedString
{
    char c_str[Size] = {0};

    constexpr FixedString() : c_str{0} {};
    constexpr FixedString(const char *str)
    {
        unsigned long idx = 0;
        const char *p = str;

        while (*p != '\0' && idx < Size)
        {
            c_str[idx] = *p;
            ++idx;
            ++p;
        }
    }

    constexpr FixedString(const std::string_view &str)
    {
        unsigned long idx = 0;
        const char *p = str.data();

        while (idx < str.size() && idx < Size)
        {
            c_str[idx] = *p;
            ++idx;
            ++p;
        }
    }

    constexpr FixedString(const FixedString<Size> &str)
    {
        for (unsigned long i = 0; i < Size; ++i)
            c_str[i] = str.c_str[i];
    }

    constexpr FixedString(FixedString<Size> &&str) noexcept
    {
        for (unsigned long i = 0; i < Size; ++i)
            c_str[i] = str.c_str[i];
    }

    constexpr ~FixedString()
    {
    }

    constexpr operator std::string_view() const
    {
        return {c_str, Size - 1};
    }

    constexpr FixedString &operator=(const FixedString<Size> &str)
    {
        for (unsigned long i = 0; i < Size; ++i)
            c_str[i] = str.c_str[i];
        return *this;
    }
};

struct Block
{
    std::array<std::pair<FixedString<FIXED_STRING_SIZE>, FixedString<FIXED_STRING_SIZE>>, MAX_BLOCK_FIELDS> fields{};
    std::size_t count{0};
};

constexpr const char datatype[] = {
#embed "NewType.dt"
    , 0};

consteval std::size_t count_block(const std::string_view &data)
{
    std::size_t n{0};

    for (char c : data)
        if (c == '[')
            ++n;

    return n;
}

consteval Block parse_block(std::size_t &idx)
{
    Block block{};

    ++idx; // skip opening '['
    std::size_t start = idx;
    std::string_view data{datatype};

    while (idx < data.size() && data[idx] != ']')
    {
        if (data[idx] == '\n')
        {
            std::string_view line{data.data() + start, idx - start};
            std::size_t ls = line.find_first_not_of(" \t");
            if (ls != std::string_view::npos)
            {
                line.remove_prefix(ls);
                std::size_t colon = line.find_first_of(':');
                std::string_view name = line.substr(0, colon);
                std::size_t name_end = name.find_last_not_of(" \t");
                name = (name_end == std::string_view::npos) ? std::string_view{} : name.substr(0, name_end + 1);
                std::size_t ts = line.find_first_not_of(" \t", colon + 1);
                std::string_view type = (ts == std::string_view::npos) ? std::string_view{} : line.substr(ts);
                std::size_t type_end = type.find_last_not_of(" \t");
                type = (type_end == std::string_view::npos) ? std::string_view{} : type.substr(0, type_end + 1);
                block.fields.at(block.count++) = {FixedString<FIXED_STRING_SIZE>(name),
                                                  FixedString<FIXED_STRING_SIZE>(type)};
            }
            start = idx + 1;
        }
        ++idx;
    }

    if (idx < data.size() && data[idx] == ']')
        ++idx; // advance past closing ']'

    return block;
}

template <std::size_t N> consteval auto parse(const std::string_view &data) -> std::array<Block, N>
{
    std::array<Block, N> output{};
    std::size_t block_idx{0};

    for (std::size_t i = 0; i < data.size(); ++i)
    {
        if (data.at(i) == '[')
            output.at(block_idx++) = parse_block(i);
    }

    return output;
}

consteval std::meta::info sv_to_mi(std::string_view type)
{
    if (type == "int")
        return ^^int;
    if (type == "float")
        return ^^float;
    if (type == "double")
        return ^^double;
    if (type == "bool")
        return ^^bool;

    throw consteval_error("sv_to_mi: unknown type", type);
}

template <Block block> consteval std::meta::info block_to_mi()
{
    struct Temp;

    consteval
    {
        std::array<std::meta::info, block.count> temp_types{};
        for (std::size_t i = 0; i < block.count; ++i)
        {
            temp_types.at(i) = std::meta::data_member_spec(sv_to_mi(block.fields[i].second.c_str),
                                                           {.name = block.fields[i].first.c_str});
        }

        std::meta::define_aggregate(^^Temp, temp_types);
    }

    return ^^Temp;
}

template <std::size_t N, std::array<Block, N> blocks> consteval auto parse_types() -> std::array<std::meta::info, N>
{
    std::array<std::meta::info, N> types{};

    template for (constexpr auto i : std::views::iota(std::size_t{0}, N)) types[i] = block_to_mi<blocks[i]>();

    return types;
}

template <typename T> constexpr void xray()
{
    template for (constexpr auto member : std::define_static_array(
                      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::current())))
    {
        std::println("{}", std::meta::identifier_of(member));
    }
}

int main()
{
    constexpr auto n = count_block(datatype);
    constexpr auto blocks = parse<n>(datatype);

    for (std::size_t i = 0; i < n; ++i)
    {
        std::println("Block {}", i);
        std::println("----------------");
        for (std::size_t f = 0; f < blocks[i].count; ++f)
        {
            std::println("{} {}", blocks[i].fields[f].first.c_str, blocks[i].fields[f].second.c_str);
        }
        std::println("----------------\n");
    }

    static constexpr auto types = parse_types<n, blocks>();

    using T1 = typename[:types[0]:];

    template for (constexpr auto type : types)
    {
        xray<typename[:type:]>();
        std::println("");
    }

    T1 t1{2, 2.0};

    std::println("{} {}", t1.first, t1.second);
}