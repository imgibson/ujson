# µJson

## Overview

Lightweight Json parsing in C++.

## Memory Management

This library does not allocate memory on the heap. Instead, it parses Json documents in-place, using string-views to reference the original text, which must remain valid for the lifetime of the parsed document. This design allows for efficient parsing without the overhead of dynamic memory allocation, but it also means that the original text must not be modified or deallocated while the parsed document is in use.

## Value Types

Since values are represented as string-views into the original text, they are only type checked during parsing but not converted. You can query the type of a value using the `isNull()`, `isBool()`, `isString()`, `isIntegral()`, `isSigned()`, `isFloat()`, `isArray()` and `isObject()` methods. Once you know the type of a value, you can retrieve it using the appropriate `as*()` method. If the value is not of the expected type or not convertible to the expected type, the `as*()` method will return an empty `std::optional` / `std::nullopt`.

## String Values

When using `asStringView()`, values are returned as an `std::string_view` into the original text (excluding the surrounding quotes), and they are not unescaped. This means that if a string value contains escape sequences (e.g., `\n`, `\t`, `\"`), they will be returned as-is in the string-view, the same applies to unicode escape sequences (e.g., `\uXXXX`). If you need to access the unescaped string value, you can use the `asString()` method, which will return a `std::string` with all escape sequences processed, if any errors are found, the method will return an empty `std::optional` / `std::nullopt`. Keep in mind that using `asString()` does in fact involve allocating memory on the heap both for temorary data and for the resulting string.

## Example Usage

```cpp
// Open a file containing Json
std::ifstream file(filename, std::ifstream::ate | std::ifstream::binary);
if (!file) return;

// Get the file size
std::ifstream::pos_type size = file.tellg();
file.seekg(0, std::ifstream::beg);

// Read file into memory
std::vector<char> text(size);
if (!file.read(&text[0], size)) return;

// Prepare a Json reader
std::optional<ujson::Reader> reader = ujson::Reader::create(text, size);
if (!reader) return;

// Parse the document
std::optional<ujson::Value> root = reader->parse();
if (!root) return;

// Print the first object (if it exists)
if (root->isObject()) {
  // Iterate one level deep into the object
  for (ujson::Reader::ObjectIterator iter(*root); iter.valid(); iter.next()) {
    const ujson::Member& member = iter.member();
    const ujson::Value& content = member.value();
    std::string memberName = member.asString().value_or(member.asStringView());
    if (content.isNull()) {
      std::cout << memberName << ": null" << std::endl;
    } else if (content.isBool()) {
      std::cout << memberName << ": " << content.asBool() << std::endl;
    } else if (content.isString()) {
      std::cout << memberName << ": \"" << content.asString().value_or(content.asStringView()) << "\"" << std::endl;
    } else if (content.isUnsigned()) {
      std::cout << memberName << ": " << content.asNumber<unsigned int>().value_or(0) << std::endl;
    } else if (content.isIntegral()) {
      std::cout << memberName << ": " << content.asNumber<int>().value_or(0) << std::endl;
    } else if (content.isFloat()) {
      std::cout << memberName << ": " << content.asNumber<float>().value_or(0.0f) << std::endl;
    } else if (content.isArray()) {
      std::cout << memberName << ": [ ... ]" << std::endl;
    } else if (content.isObject()) {
      std::cout << memberName << ": { ... }" << std::endl;
    }
  }
}
```
