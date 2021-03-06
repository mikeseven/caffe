/*
 * device_context.cpp
 *
 *  Created on: Jun 26, 2015
 *      Author: Fabian Tschopp
 */

#include <algorithm>
#include <string>
#include <vector>
#include "caffe/device_context.hpp"
#include "caffe/greentea/greentea.hpp"
#include "caffe/util/device_alternate.hpp"

#ifdef USE_GREENTEA
#include "caffe/greentea/cl_kernels.hpp"
#endif  // USE_GREENTEA

namespace caffe {

DeviceContext::DeviceContext()
    : current_queue_id_(0), workgroup_sizes_(3, 0), id_(0), list_id_(0),
      backend_(Backend::BACKEND_CPU), memory_usage_(0), peak_memory_usage_(0) {
}

DeviceContext::DeviceContext(int id, int list_id, Backend backend)
    : current_queue_id_(0), workgroup_sizes_(3, 0), id_(id), list_id_(list_id),
      backend_(backend), memory_usage_(0), peak_memory_usage_(0) {
}

void DeviceContext::Init() {
#ifndef CPU_ONLY
  if (backend_ == BACKEND_CUDA) {
#ifdef USE_CUDA
    workgroup_sizes_[0] = CAFFE_CUDA_NUM_THREADS;
#endif  // USE_CUDA
  } else {
#ifdef USE_GREENTEA
    viennacl::ocl::context &ctx = viennacl::ocl::get_context(id_);

    std::vector<size_t> temp(3);
    clGetDeviceInfo(ctx.devices()[0].id(),
    CL_DEVICE_MAX_WORK_ITEM_SIZES,
                    sizeof(size_t), &temp[0], NULL);
    workgroup_sizes_[0] = temp[0];
    workgroup_sizes_[1] = temp[1];
    workgroup_sizes_[2] = temp[2];

    SetProgram();

    for (int q = 0; q < GREENTEA_QUEUE_COUNT - 1; ++q) {
      ctx.add_queue(ctx.devices()[0]);
    }
#endif  // USE_GREENTEA
  }
#endif  // !CPU_ONLY
}

Backend DeviceContext::backend() const {
  return backend_;
}

int DeviceContext::id() const {
  return id_;
}

int DeviceContext::list_id() const {
  return list_id_;
}

int DeviceContext::WorkgroupSize(int id) {
  return workgroup_sizes_[id];
  return 0;
}

int DeviceContext::num_queues() {
  if (backend_ == BACKEND_CUDA) {
#ifdef USE_CUDA
    return 1;
#endif  // USE_CUDA
  } else {
#ifdef USE_GREENTEA
    return GREENTEA_QUEUE_COUNT;
#endif  // USE_GREENTEA
  }
  return 1;
}

template<>
shared_ptr<Blob<float> > DeviceContext::Buffer(int id) {
  if (buff_f_.size() <= id) {
    shared_ptr<Blob<float> > blob_pointer(new Blob<float>(this));
    buff_f_.push_back(blob_pointer);
  }
  return buff_f_[id];
}

template<>
shared_ptr<Blob<double> > DeviceContext::Buffer(int id) {
  if (buff_d_.size() <= id) {
    shared_ptr<Blob<double> > blob_pointer(new Blob<double>(this));
    buff_d_.push_back(blob_pointer);
  }
  return buff_d_[id];
}

int DeviceContext::current_queue_id() {
  return current_queue_id_;
}

void DeviceContext::SwitchQueue(int id) {
  if (backend_ == BACKEND_CUDA) {
#ifdef USE_CUDA
    (void) id;
#endif  // USE_CUDA
  } else {
#ifdef USE_GREENTEA
    viennacl::ocl::context &ctx = viennacl::ocl::get_context(id_);
    ctx.switch_queue(id % num_queues());
    current_queue_id_ = id % num_queues();
#endif  // USE_GREENTEA
  }
}

void DeviceContext::FinishQueues() {
  if (backend_ == BACKEND_CUDA) {
#ifdef USE_CUDA
#endif  // USE_CUDA
  } else {
#ifdef USE_GREENTEA
    viennacl::ocl::context &ctx = viennacl::ocl::get_context(id_);
    for (int i = 0; i < num_queues(); ++i) {
      ctx.switch_queue(i);
      ctx.get_queue().finish();
    }
    ctx.switch_queue(0);
    current_queue_id_ = 0;
#endif  // USE_GREENTEA
  }
}

size_t DeviceContext::memory_usage() {
  return memory_usage_;
}

size_t DeviceContext::peak_memory_usage() {
  return peak_memory_usage_;
}

void DeviceContext::IncreaseMemoryUsage(size_t bytes) {
  memory_usage_ += bytes;
  if (memory_usage_ > peak_memory_usage_) {
    peak_memory_usage_ = memory_usage_;
  }
}

void DeviceContext::DecreaseMemoryUsage(size_t bytes) {
  memory_usage_ -= bytes;
}

void DeviceContext::ResetPeakMemoryUsage() {
  peak_memory_usage_ = memory_usage_;
}

bool DeviceContext::CheckCapability(std::string cap) {
  if (backend_ == BACKEND_OpenCL) {
#ifdef USE_GREENTEA
    viennacl::ocl::context &ctx = viennacl::ocl::get_context(id_);

    size_t size;
    size_t max_size = 1024 * 1024;
    clGetDeviceInfo(ctx.devices()[0].id(), CL_DEVICE_EXTENSIONS,
                    0, NULL, &size);

    // Cap at 1 MB to capture faulty OpenCL implementations (nVidia)
    std::vector<char> exts(std::min(size, max_size));

    clGetDeviceInfo(ctx.devices()[0].id(), CL_DEVICE_EXTENSIONS,
                    size, &(exts[0]), NULL);

    std::string extsstr(&(exts[0]));
    return extsstr.find(cap) != std::string::npos;
#endif
  }
  return true;
}

#ifdef USE_GREENTEA
viennacl::ocl::program &DeviceContext::program() {
  return ocl_program_;
}

void DeviceContext::SetProgram() {
  ocl_program_ = RegisterKernels(
      &(viennacl::ocl::get_context(static_cast<uint64_t>(id_))));
}


#endif  // USE_GREENTEA

}  // namespace caffe
