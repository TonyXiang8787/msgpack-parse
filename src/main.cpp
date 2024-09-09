#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <msgpack.hpp>
#include <numbers>
#include <sstream>

msgpack::sbuffer create_msg(size_t size) {
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> pk(&sbuf);
    double const value = std::numbers::pi;
    for (size_t i = 0; i < size; i++) {
        pk.pack(i);
    }
    return sbuf;
}

struct DefaultNullVisitor : msgpack::null_visitor {
    static std::string msg_for_parse_error(size_t parsed_offset, size_t error_offset, std::string_view msg) {
        std::stringstream ss;
        ss << msg << ", parsed_offset: " << parsed_offset << ", error_offset: " << error_offset << ".\n";
        return ss.str();
    }

    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    [[noreturn]] void parse_error(size_t parsed_offset, size_t error_offset) {
        throw std::runtime_error{msg_for_parse_error(parsed_offset, error_offset, "Error in parsing")};
    }
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    [[noreturn]] void insufficient_bytes(size_t parsed_offset, size_t error_offset) {
        throw std::runtime_error{msg_for_parse_error(parsed_offset, error_offset, "Insufficient bytes")};
    }
};

template <class T> struct DefaultErrorVisitor : DefaultNullVisitor {
    static constexpr std::string_view static_err_msg = "Unexpected data type!\n";

    bool visit_nil() { return throw_error(); }
    bool visit_boolean(bool /*v*/) { return throw_error(); }
    bool visit_positive_integer(uint64_t /*v*/) { return throw_error(); }
    bool visit_negative_integer(int64_t /*v*/) { return throw_error(); }
    bool visit_float32(float /*v*/) { return throw_error(); }
    bool visit_float64(double /*v*/) { return throw_error(); }
    bool visit_str(const char* /*v*/, uint32_t /*size*/) { return throw_error(); }
    bool visit_bin(const char* /*v*/, uint32_t /*size*/) { return throw_error(); }
    bool visit_ext(const char* /*v*/, uint32_t /*size*/) { return throw_error(); }
    bool start_array(uint32_t /*num_elements*/) { return throw_error(); }
    bool start_array_item() { return throw_error(); }
    bool end_array_item() { return throw_error(); }
    bool end_array() { return throw_error(); }
    bool start_map(uint32_t /*num_kv_pairs*/) { return throw_error(); }
    bool start_map_key() { return throw_error(); }
    bool end_map_key() { return throw_error(); }
    bool start_map_value() { return throw_error(); }
    bool end_map_value() { return throw_error(); }
    bool end_map() { return throw_error(); }

    bool throw_error() { throw std::runtime_error{(static_cast<T&>(*this)).get_err_msg()}; }

    std::string get_err_msg() { return std::string{T::static_err_msg}; }
};

struct DoubleVisitor : DefaultErrorVisitor<DoubleVisitor> {
    static constexpr std::string_view static_err_msg = "Expect a number.";

    double& value; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

    bool visit_nil() { return true; } // NOLINT(readability-convert-member-functions-to-static)
    bool visit_positive_integer(uint64_t v) {
        value = static_cast<double>(v);
        return true;
    }
    bool visit_negative_integer(int64_t v) {
        value = static_cast<double>(v);
        return true;
    }
    bool visit_float32(float v) {
        value = v;
        return true;
    }
    bool visit_float64(double v) {
        value = v;
        return true;
    }
};

int main() {
#ifdef NDEBUG
    constexpr size_t size = 10_000_000;
#else
    constexpr size_t size = 10;
#endif
    auto const sbuf = create_msg(size);
    return 0;
}
