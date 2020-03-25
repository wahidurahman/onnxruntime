// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "cuda_common.h"
#include "core/framework/tensor.h"
#include <cfloat>

namespace onnxruntime {
namespace cuda {

class CudnnTensor final {
 public:
  CudnnTensor();
  ~CudnnTensor();
  CudnnTensor(const CudnnTensor&) = delete;
  CudnnTensor& operator=(const CudnnTensor&) = delete;

  Status Set(const std::vector<int64_t>& input_dims, cudnnDataType_t dataType);
  Status Set(const CudnnTensor& x_desc, cudnnBatchNormMode_t mode);

  operator cudnnTensorDescriptor_t() const { return tensor_; }

  template <typename T>
  static cudnnDataType_t GetDataType();

 private:
  Status CreateTensorIfNeeded();

  cudnnTensorDescriptor_t tensor_;
};

class CudnnDataTensor final {
 public:
  CudnnDataTensor();
  ~CudnnDataTensor();
  CudnnDataTensor(const CudnnDataTensor&) = delete;
  CudnnDataTensor& operator=(const CudnnDataTensor&) = delete;

  Status Set(cudnnDataType_t dataType,
             int64_t max_seq_length,
             int64_t batch_size,
             int64_t data_size,
             const int32_t* seq_lengths);

  operator cudnnRNNDataDescriptor_t() const { return tensor_; }

 private:
  Status CreateTensorIfNeeded();

  cudnnRNNDataDescriptor_t tensor_;
};

class CudnnFilterDescriptor final {
 public:
  CudnnFilterDescriptor();
  ~CudnnFilterDescriptor();
  CudnnFilterDescriptor(const CudnnFilterDescriptor&) = delete;
  CudnnFilterDescriptor& operator=(const CudnnFilterDescriptor&) = delete;

  Status Set(const std::vector<int64_t>& filter_dims, cudnnDataType_t data_typ);

  operator cudnnFilterDescriptor_t() const { return desc_; }

 private:
  cudnnFilterDescriptor_t desc_;
};

class CudnnDropout final {
 public:
  CudnnDropout() : dropout_desc_(nullptr) {
  }
  CudnnDropout(const CudnnDropout&) = delete;
  CudnnDropout& operator=(const CudnnDropout&) = delete;

  Status GetCudnnDropoutStatesSize(const cudnnHandle_t& cudnnHandle, size_t& stateSize) {
    CUDNN_RETURN_IF_ERROR(cudnnDropoutGetStatesSize(cudnnHandle, &stateSize));

    return Status::OK();
  }

  Status Set(const cudnnHandle_t& cudnnHandle,
             void* states,
             size_t stateSize,
             float dropout = 0.0f,
             unsigned long long seed = 1) {
    ORT_RETURN_IF_ERROR(CreateDescriptorIfNeeded());
    CUDNN_RETURN_IF_ERROR(cudnnSetDropoutDescriptor(dropout_desc_,
                                                    cudnnHandle,
                                                    dropout,
                                                    states,
                                                    stateSize,
                                                    seed));

    return Status::OK();
  }

  ~CudnnDropout() {
    if (dropout_desc_ != nullptr) {
      cudnnDestroyDropoutDescriptor(dropout_desc_);
    }
  }

  operator cudnnDropoutDescriptor_t() const {
    return dropout_desc_;
  }

  Status CreateDescriptorIfNeeded() {
    if (!dropout_desc_)
      CUDNN_RETURN_IF_ERROR(cudnnCreateDropoutDescriptor(&dropout_desc_));
    return Status::OK();
  }

 private:
  cudnnDropoutDescriptor_t dropout_desc_;
};

template <typename ElemType>
struct Consts {
  static const ElemType Zero;
  static const ElemType One;
};

template <>
struct Consts<half> {
  static const float Zero;
  static const float One;
};

inline double ClampCudnnBatchNormEpsilon(double epsilon) {
  if (epsilon < CUDNN_BN_MIN_EPSILON) {
    if (CUDNN_BN_MIN_EPSILON - epsilon > FLT_EPSILON)
      LOGS_DEFAULT(WARNING) << "Provided epsilon is smaller than CUDNN_BN_MIN_EPSILON. Setting it to CUDNN_BN_MIN_EPSILON";
    return CUDNN_BN_MIN_EPSILON;
  }
  return epsilon;
}

}  // namespace cuda
}  // namespace onnxruntime
