#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <msgpack.hpp>
#include <numbers>
#include <numeric>
#include <sstream>
#include <vector>

msgpack::sbuffer create_msg(size_t size) {
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> pk(&sbuf);
    double const value = std::numbers::pi;
    for (size_t i = 0; i < size; i++) {
        pk.pack(value);
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

class Deserializer {
  public:
    Deserializer(char const* data, size_t length, size_t size)
        : data_{data}, length_{length}, size_{size}, values_(size) {}

    void parse_all() {
        offset_ = 0;
        for (size_t i = 0; i < size_; i++) {
            parse_double(i);
        }
    }

    void skip_all() {
        offset_ = 0;
        for (size_t i = 0; i < size_; i++) {
            parse_skip(i);
        }
    }

    double sum_all() { return std::reduce(values_.cbegin(), values_.cend()); }

  private:
    char const* data_;
    size_t length_;
    size_t size_;
    size_t offset_{};
    std::vector<double> values_;

    void parse_skip(size_t /* pos */) {
        DefaultNullVisitor visitor{};
        msgpack::parse(data_, length_, offset_, visitor);
    }

    void parse_double(size_t pos) {
        DoubleVisitor visitor{.value = values_[pos]};
        msgpack::parse(data_, length_, offset_, visitor);
    }
};

int main() {
#ifdef NDEBUG
    constexpr size_t size = 10_000_000;
#else
    constexpr size_t size = 10;
#endif
    auto const sbuf = create_msg(size);
    Deserializer deserializer(sbuf.data(), sbuf.size(), size);

    auto start = std::chrono::high_resolution_clock::now();
    deserializer.skip_all();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Time taken to skip: " << duration.count() << " seconds\n";

    start = std::chrono::high_resolution_clock::now();
    deserializer.parse_all();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Time taken to parse: " << duration.count() << " seconds\n";

    std::cout << "Sum: " << deserializer.sum_all() << '\n';
    return 0;
}
