#ifndef CAFFE_SYNCEDMEM_HPP_
#define CAFFE_SYNCEDMEM_HPP_

#include <cstdlib>

#include "caffe/common.hpp"
#include "caffe/greentea/greentea.hpp"
#include "caffe/util/math_functions.hpp"

#define OPENCL_PAGE_ALIGN 4096
#define OPENCL_CACHE_ALIGN 64

namespace caffe {

void CaffeMallocHost(void** ptr, size_t size);

void CaffeFreeHost(void* ptr);

/**
 * @brief Manages memory allocation and synchronization between the host (CPU)
 *        and device (GPU).
 *
 * TODO(dox): more thorough description.
 */
class SyncedMemory {
 public:
#ifdef USE_GREENTEA
  SyncedMemory()
      : cpu_ptr_(NULL),
        gpu_ptr_(NULL),
        size_(0),
        head_(UNINITIALIZED),
        own_cpu_data_(false),
        own_gpu_data_(false),
        device_context_(Caffe::GetDefaultDeviceContext()),
        cl_gpu_mem_(NULL) {
  }
  explicit SyncedMemory(DeviceContext *device_context)
      : cpu_ptr_(NULL),
        gpu_ptr_(NULL),
        size_(0),
        head_(UNINITIALIZED),
        own_cpu_data_(false),
        own_gpu_data_(false),
        device_context_(device_context),
        cl_gpu_mem_(NULL) {
  }
  explicit SyncedMemory(size_t size, DeviceContext *device_context)
      : cpu_ptr_(NULL),
        gpu_ptr_(NULL),
        size_(size),
        head_(UNINITIALIZED),
        own_cpu_data_(false),
        own_gpu_data_(false),
        device_context_(device_context),
        cl_gpu_mem_(NULL) {
  }
#else
  SyncedMemory()
      : cpu_ptr_(NULL),
        gpu_ptr_(NULL),
        size_(0),
        head_(UNINITIALIZED),
        own_cpu_data_(false),
        own_gpu_data_(false),
        device_context_(Caffe::GetDefaultDeviceContext()) {
  }
  explicit SyncedMemory(DeviceContext *device_context)
      : cpu_ptr_(NULL),
        gpu_ptr_(NULL),
        size_(0),
        head_(UNINITIALIZED),
        own_cpu_data_(false),
        own_gpu_data_(false),
        device_context_(device_context) {
  }
  explicit SyncedMemory(size_t size, DeviceContext *device_context)
      : cpu_ptr_(NULL),
        gpu_ptr_(NULL),
        size_(size),
        head_(UNINITIALIZED),
        own_cpu_data_(false),
        own_gpu_data_(false),
        device_context_(device_context) {
  }
#endif

  ~SyncedMemory();
  const void* cpu_data();
  void set_cpu_data(void* data);
  const void* gpu_data();
  void set_gpu_data(void* data);
  void* mutable_cpu_data();
  void* mutable_gpu_data();
  enum SyncedHead {
    UNINITIALIZED,
    HEAD_AT_CPU,
    HEAD_AT_GPU,
    SYNCED
  };
  SyncedHead head() {
    return head_;
  }
  size_t size() {
    return size_;
  }

#ifndef CPU_ONLY
#ifdef USE_CUDA
  void async_gpu_push(const cudaStream_t& stream);
#endif  // USE_CUDA
#endif  // !CPU_ONLY

 private:
  void to_cpu();
  void to_gpu();
  void* cpu_ptr_;
  void* gpu_ptr_;

  size_t size_;
  SyncedHead head_;
  bool own_cpu_data_;
  bool own_gpu_data_;
  DeviceContext *device_context_;

#ifdef USE_GREENTEA
  cl_mem cl_gpu_mem_;
#endif


DISABLE_COPY_AND_ASSIGN(SyncedMemory);
};
// class SyncedMemory

}  // namespace caffe

#endif  // CAFFE_SYNCEDMEM_HPP_
