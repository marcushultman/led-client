#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace led {

struct SetOptions {
  double src = 1.0;
  double dst = 1.0;
};

struct Buffer {
  virtual ~Buffer() = default;

  virtual void clear() = 0;
  virtual void set(size_t i, uint8_t r, uint8_t g, uint8_t b, const SetOptions& options = {}) = 0;

  virtual uint8_t* data() = 0;
  virtual size_t size() const = 0;
};

struct LED {
  virtual ~LED() = default;
  virtual std::unique_ptr<Buffer> createBuffer() = 0;
  virtual void show(Buffer&) = 0;
};

}  // namespace led
