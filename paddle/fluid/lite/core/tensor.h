// Copyright (c) 2019 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#include <algorithm>
#include <memory>
#include <numeric>
#include <vector>
#include "paddle/fluid/lite/core/memory.h"
#include "paddle/fluid/lite/core/target_wrapper.h"

namespace paddle {
namespace lite {

using DDim = std::vector<int64_t>;
static DDim SliceDims(const DDim& dims, int begin, int end) {
  return DDim(dims.begin() + begin, dims.begin() + end - 1);
}

static int product(const DDim& dims) {
  return std::accumulate(dims.begin(), dims.end(), 1,
                         [](int a, int b) { return a * b; });
}

static int product(DDim::const_iterator begin, DDim::const_iterator end) {
  return std::accumulate(begin, end, 1, [](int a, int b) { return a * b; });
}

static DDim flatten_to_2d(const DDim& dims, int col) {
  return DDim({product(SliceDims(dims, 0, col)),
               product(SliceDims(dims, col, dims.size()))});
}

using LoD = std::vector<std::vector<size_t>>;

// A light-weight tensor implementation.
class Tensor {
 public:
  Tensor() : buffer_(std::make_shared<Buffer>()) {}

  template <typename T>
  const T* data() const {
    return static_cast<const T*>(buffer_->data());
  }

  void Resize(const DDim& ddim) { dims_ = ddim; }

  const DDim& dims() const { return dims_; }

  const LoD& lod() { return lod_; }
  LoD* mutable_lod() { return &lod_; }

  template <typename T>
  T* mutable_data() {
    memory_size_ = product(dims_) * sizeof(T);
    buffer_->ResetLazy(target_, memory_size_);
    return static_cast<T*>(buffer_->data());
  }

  template <typename T>
  T* mutable_data(TargetType target) {
    target_ = target;
    buffer_->ResetLazy(target, memory_size());
    return static_cast<T*>(buffer_->data());
  }

  void* mutable_data(size_t memory_size) {
    buffer_->ResetLazy(target_, memory_size);
    return buffer_->data();
  }

  void* mutable_data(TargetType target, size_t memory_size) {
    target_ = target;
    return mutable_data(memory_size);
  }

  size_t memory_size() const { return memory_size_; }

  bool IsInitialized() const { return buffer_->data(); }

  // Other share data to this.
  void ShareDataWith(const Tensor& other) {
    buffer_ = other.buffer_;
    dims_ = other.dims_;
    target_ = other.target_;
    lod_ = other.lod_;
  }

  void CopyDataFrom(const Tensor& other) {
    dims_ = other.dims_;
    target_ = other.target_;
    lod_ = other.lod_;
    *buffer_ = *other.buffer_;
  }

  TargetType target() const { return target_; }

 private:
  TargetType target_{TargetType::kHost};
  DDim dims_;
  std::shared_ptr<Buffer> buffer_;
  LoD lod_;
  size_t memory_size_{};
};

std::ostream& operator<<(std::ostream& os, const DDim& dims);
std::ostream& operator<<(std::ostream& os, const Tensor& tensor);

}  // namespace lite
}  // namespace paddle
