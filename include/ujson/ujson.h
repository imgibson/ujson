/**
 * 
 * @author Anders Lind (96395432+imgibson@users.noreply.github.com)
 * @date 2026-02-14
 * 
 * Copyright (c) 2026 Anders Lind (https://github.com/imgibson). All rights reserved.
 * 
 */

#pragma once

#include <cassert>
#include <charconv>
#include <optional>
#include <string_view>
#include <type_traits>

namespace ujson {

class Value {
public:
  /// Copy from another object.
  Value(const Value& copy) noexcept
    : type_{copy.type_}
    , beg_{copy.beg_}
    , end_{copy.end_} {}

  /// Check if this is a null value.
  bool isNull() const noexcept {
    return type_ == Type::Null;
  }

  /// Check if this is a boolean value type.
  bool isBool() const noexcept {
    return type_ == Type::True || type_ == Type::False;
  }

  /// Check if this is a number value type.
  bool isNumber() const noexcept {
    return isFloat() || isIntegral();
  }

  /// Check if this is a floating point number value type.
  bool isFloat() const noexcept {
    return type_ == Type::Float;
  }

  /// Check if this is a integer number value type.
  bool isIntegral() const noexcept {
    return type_ == Type::Signed || isUnsigned();
  }

  /// Check if this is a unsigned integer number value type.
  bool isUnsigned() const noexcept {
    return type_ == Type::Unsigned;
  }

  /// Check if this is a string value type.
  bool isString() const noexcept {
    return type_ == Type::String;
  }

  /// Check if this is an array value type.
  bool isArray() const noexcept {
    return type_ == Type::Array;
  }

  /// Check if this is an object value type.
  bool isObject() const noexcept {
    return type_ == Type::Object;
  }

  /// Retrieve the raw string view of the value.
  std::string_view asData() const noexcept {
    return {beg_, static_cast<std::string_view::size_type>(end_ - beg_)};
  }

  /// Try to retrieve the value as a boolean value type.
  std::optional<bool> asBool() const noexcept {
    if (!isBool()) {
      return std::nullopt;
    }
    return {type_ == Type::True};
  }

  /// Try to retrieve the value as a number value type.
  template <typename Type>
  std::optional<Type> asNumber() const noexcept
    requires (std::is_arithmetic_v<Type> && sizeof(T) <= 4) {
    assert(isNumber());
    Type number;
    if (!isNumber() || std::from_chars(beg_, end_, number).ec != std::errc()) {
      return std::nullopt;
    }
    return {number};
  }

  /// Try to retrieve the value as a string view value type.
  std::optional<std::string_view> asString() const noexcept {
    if (!isString()) {
      return std::nullopt;
    }
    return {{&beg_[1], static_cast<std::string_view::size_type>(&end_[-1] - &beg_[1])}};
  }

private:
  friend class Reader;

  struct Type {
    enum Enum {
      Null,
      True,
      False,
      Float,
      Signed,
      Unsigned,
      String,
      Array,
      Object
    };
  };

  Value(Type::Enum type, const char* beg, const char* end) noexcept
    : type_{type}
    , beg_{beg}
    , end_{end} {}

  Type::Enum type_;
  const char* beg_;
  const char* end_;
};

class Member {
public:
  std::string_view name() const noexcept {
    return {&beg_[1], static_cast<std::string_view::size_type>(&end_[-1] - &beg_[1])};
  }

  const Value& value() const noexcept {
    return value_;
  }

private:
  friend class Reader;

  Member(const char* beg, const char* end, const Value& value) noexcept
    : beg_{beg}
    , end_{end}
    , value_{value} {}

  const char* beg_;
  const char* end_;
  Value value_;
};

class Reader {
public:
  class ObjectIterator {
  public:
    ObjectIterator(const Value& value) noexcept
      : ObjectIterator(value.beg_, value.end_) {}

    void next() noexcept {
      if (member_.has_value()) {
        const char* beg = skipWhitespaces(member_->value().end_, end_);
        if (beg < end_ && *beg++ == ',') {
          beg = skipWhitespaces(beg, end_);
          if (beg < end_ && *beg == '"') {
            std::optional<Value> name = parseString(beg, end_);
            if (name.has_value()) {
              beg = skipWhitespaces(name->end_, end_);
              if (beg < end_ && *beg++ == ':') {
                std::optional<Value> value = parseValue(skipWhitespaces(beg, end_), end_);
                if (value.has_value()) {
                  member_ = Member{name->beg_, name->end_, *value};
                  return;
                }
              }
            }
          }
        }
        member_ = std::nullopt;
      }
    }

    bool valid() const noexcept {
      return member_.has_value();
    }

    const Member& member() const noexcept {
      return member_.value();
    }

  private:
    friend class Reader;

    ObjectIterator(const char* beg, const char* end) noexcept
      : end_{end}
      , member_{std::nullopt} {
      assert(*beg == '{');
      beg = skipWhitespaces(++beg, end_);
      if (beg < end_ && *beg == '"') {
        std::optional<Value> name = parseString(beg, end);
        if (name.has_value()) {
          beg = skipWhitespaces(name->end_, end_);
          if (beg < end && *beg++ == ':') {
            std::optional<Value> value = parseValue(skipWhitespaces(beg, end_), end);
            if (value.has_value()) {
              member_ = Member{name->beg_, name->end_, *value};
            }
          }
        }
      }
    }

    const char* end_;
    std::optional<Member> member_;
  };

  class ArrayIterator {
  public:
    ArrayIterator(const Value& value) noexcept
      : ArrayIterator(value.beg_, value.end_) {}

    void next() noexcept {
      if (value_.has_value()) {
        const char* beg = skipWhitespaces(value_->end_, end_);
        if (beg < end_ && *beg++ == ',') {
          value_ = parseValue(skipWhitespaces(beg, end_), end_);
          return;
        }
        value_ = std::nullopt;
      }
    }

    bool valid() const noexcept {
      return value_.has_value();
    }

    const Value& value() const noexcept {
      return value_.value();
    }

  private:
    friend class Reader;

    ArrayIterator(const char* beg, const char* end) noexcept
      : end_{end}
      , value_{std::nullopt} {
      assert(*beg == '[');
      value_ = parseValue(skipWhitespaces(++beg, end_), end_);
    }

    const char* end_;
    std::optional<Value> value_;
  };

  static std::optional<Reader> create(const char* beg, const char* end) noexcept {
    if (beg == nullptr || beg > end) {
      return std::nullopt;
    }
    return {{beg, end}};
  }

  static std::optional<Reader> create(const char* json, std::size_t size) noexcept {
    if (json == nullptr) {
      return std::nullopt;
    }
    return create(json, json + size);
  }

  std::optional<Value> parse() const noexcept {
    return parseValue(skipWhitespaces(beg_, end_), end_);
  }

private:
  const char* beg_;
  const char* end_;

  Reader(const char* beg, const char* end) noexcept
    : beg_{beg}
    , end_{end} {}

  static constexpr std::optional<Value> parseValue(const char* cur, const char* end) noexcept {
    if (cur >= end) return std::nullopt;
    switch (*cur) {
      case '"':
        return parseString(cur, end);
      case '-':
        return parseSigned(cur, end);
      case '0':
        return parseUnsigned0(cur, cur, end);
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return parseUnsigned1to9(cur, cur, end);
      case '[':
        return parseScope<Value::Type::Array, '[', ']'>(cur, end);
      case '{':
        return parseScope<Value::Type::Object, '{', '}'>(cur, end);
      case 'n':
        return parseChars<Value::Type::Null>({'n', 'u', 'l', 'l'}, cur, end);
      case 't':
        return parseChars<Value::Type::True>({'t', 'r', 'u', 'e'}, cur, end);
      case 'f':
        return parseChars<Value::Type::False>({'f', 'a', 'l', 's', 'e'}, cur, end);
    }
    return std::nullopt;
  }

  template <Value::Type::Enum Type, char Beg, char End>
  static constexpr std::optional<Value> parseScope(const char* cur, const char* end) noexcept
    requires (Type == Value::Type::Array || Type == Value::Type::Object) {
    assert(*cur == Beg);
    const char* beg = cur++;
    int depth = 1;
    while (cur < end) {
      if (*cur == Beg && ++depth > 64) {
        break;
      }
      if (*cur++ == End && --depth == 0) {
        return {{Type, beg, cur}};
      }
    }
    return std::nullopt;
  }

  static constexpr std::optional<Value> parseString(const char* cur, const char* end) noexcept {
    assert(*cur == '"');
    const char* beg = cur++;
    while (cur < end) {
      if (*cur == '\\') {
        if (++cur >= end) break;
        if (*cur == 'u') {
          if (++cur >= end || !isHexadecimal(*cur)) break;
          if (++cur >= end || !isHexadecimal(*cur)) break;
          if (++cur >= end || !isHexadecimal(*cur)) break;
          if (++cur >= end || !isHexadecimal(*cur)) break;
        } else if (*cur != '"' && *cur != '\\' && *cur != '/' && *cur != 'b' && *cur != 'f' && *cur != 'n' && *cur != 'r' && *cur != 't') {
          break;
        }
        if (++cur >= end) break;
      }
      if (*cur++ == '"') {
        return {{Value::Type::String, beg, cur}};
      }
    }
    return std::nullopt;
  }

  template <Value::Type::Enum Type, std::size_t N>
  static constexpr std::optional<Value> parseChars(const char (&str)[N], const char* cur, const char* end) noexcept
    requires (N > 0 && (Type == Value::Type::Null || Type == Value::Type::True || Type == Value::Type::False)) {
    assert(*cur == str[0]);
    if (static_cast<decltype(N)>(end - cur) < N) return std::nullopt;
    for (decltype(N) i = 1; i < N; ++i) {
      if (cur[i] != str[i]) {
        return std::nullopt;
      }
    }
    return {{Type, cur, &cur[N]}};
  }

  static constexpr std::optional<Value> parseFloat(const char* beg, const char* cur, const char* end) noexcept {
    assert(*cur == '.');
    if (++cur >= end || !isDigit(*cur)) return std::nullopt;
    do {
      if (++cur >= end) break;
    } while (isDigit(*cur));
    if (*cur == 'E' || *cur == 'e') {
      if (++cur >= end) return std::nullopt;
      if (*cur == '-' || *cur == '+') {
        if (++cur >= end) return std::nullopt;
      }
      if (!isDigit(*cur)) return std::nullopt;
      do {
        if (++cur >= end) break;
      } while (isDigit(*cur));
    }
    return {{Value::Type::Float, beg, cur}};
  }

  static constexpr std::optional<Value> parseSigned(const char* cur, const char* end) noexcept {
    assert(*cur == '-');
    const char* beg = cur;
    if (++cur >= end) return std::nullopt;
    return parseUnsigned1to9<Value::Type::Signed>(beg, cur, end);
  }

  template <Value::Type::Enum Type = Value::Type::Unsigned>
  static constexpr std::optional<Value> parseUnsigned0(const char* beg, const char* cur, const char* end) noexcept
    requires (Type == Value::Type::Unsigned || Type == Value::Type::Signed) {
    assert(*cur == '0');
    if (++cur >= end) return {{Type, beg, cur}};     
    if (*cur == '.') {
      return parseFloat(beg, cur, end);
    }
    return {{Type, beg, cur}};
  }

  template <Value::Type::Enum Type = Value::Type::Unsigned>
  static constexpr std::optional<Value> parseUnsigned1to9(const char* beg, const char* cur, const char* end) noexcept
    requires (Type == Value::Type::Unsigned || Type == Value::Type::Signed) {
    assert(isDigit1to9(*cur));
    do {
      if (++cur >= end) break;
    } while (isDigit(*cur));
    if (*cur == '.') {
      return parseFloat(beg, cur, end);
    }
    return {{Type, beg, cur}};
  }

  static constexpr bool isDigit(char c) noexcept {
    return c >= '0' && c <= '9';
  }

  static constexpr bool isDigit1to9(char c) noexcept {
    return c >= '1' && c <= '9';
  }

  static constexpr bool isHexadecimal(char c) noexcept {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
  }

  static constexpr const char* skipWhitespaces(const char* cur, const char* end) noexcept {
    while (cur < end && (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r')) {
      ++cur;
    }
    return cur;
  }
};

} // namespace ujson
