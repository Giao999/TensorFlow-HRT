/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef TENSORFLOW_CORE_KERNELS_ACL_OPS_HPP_
#define TENSORFLOW_CORE_KERNELS_ACL_OPS_HPP_

#if defined(USE_ACL)

#include "arm_compute/runtime/NEON/functions/NEConvolutionLayer.h"
#include "arm_compute/runtime/NEON/functions/NEDirectConvolutionLayer.h"
#include "arm_compute/runtime/CL/functions/CLConvolutionLayer.h"
#include "arm_compute/runtime/NEON/functions/NEActivationLayer.h"
#include "arm_compute/runtime/CL/functions/CLActivationLayer.h"
#include "arm_compute/runtime/NEON/functions/NENormalizationLayer.h"
#include "arm_compute/runtime/CL/functions/CLNormalizationLayer.h"
#include "arm_compute/runtime/NEON/functions/NEPoolingLayer.h"
#include "arm_compute/runtime/CL/functions/CLPoolingLayer.h"
#include "arm_compute/runtime/NEON/functions/NESoftmaxLayer.h"
#include "arm_compute/runtime/CL/functions/CLSoftmaxLayer.h"
#include "arm_compute/runtime/NEON/functions/NEFullyConnectedLayer.h"
#include "arm_compute/runtime/CL/functions/CLFullyConnectedLayer.h"
#include "arm_compute/runtime/NEON/functions/NELocallyConnectedLayer.h"
#include "arm_compute/runtime/CL/functions/CLLocallyConnectedLayer.h"
#include "arm_compute/runtime/NEON/functions/NEBatchNormalizationLayer.h"
#include "arm_compute/runtime/CL/functions/CLBatchNormalizationLayer.h"
//#include "arm_compute/core/NEON/kernels/NEDepthConcatenateKernel.h"
//#include "arm_compute/runtime/NEON/functions/NEDepthConcatenate.h"
//#include "arm_compute/core/CL/kernels/CLDepthConcatenateKernel.h"
//#include "arm_compute/runtime/CL/functions/CLDepthConcatenate.h"
#include "arm_compute/runtime/CL/CLTensor.h"
#include "arm_compute/runtime/Tensor.h"
#include "arm_compute/runtime/CL/CLScheduler.h"

#include "tensorflow/core/platform/logging.h"

#define FLAGS_ENABLE_ACL_CONV      0x00000004
#define FLAGS_ENABLE_ACL_MATMUL    0x00000008
#define FLAGS_ENABLE_ACL_LRN       0x00000010
#define FLAGS_ENABLE_ACL_POOLING   0x00000020
#define FLAGS_ENABLE_ACL_RELU      0x00000040
#define FLAGS_ENABLE_ACL_SIGMOID   0x00000080
#define FLAGS_ENABLE_ACL_SOFTMAX   0x00000100
#define FLAGS_ENABLE_ACL_TANH      0x00000200
extern int64_t bypass_acl_class_layer;

#if defined(USE_PROFILING)
#include <sys/time.h>
#define NANO_SEC_CONV 1000000

#define MASK_LOG_APP_TIME 0x00000001
#define MASK_LOG_ALLOCATE 0x00000002
#define MASK_LOG_RUN      0x00000004
#define MASK_LOG_CONFIG   0x00000008
#define MASK_LOG_COPY     0x00000010
#define MASK_LOG_ABSVAL   0x00000020
#define MASK_LOG_BNLL     0x00000040
#define MASK_LOG_CONV     0x00000080
#define MASK_LOG_MATMUL   0x00000100
#define MASK_LOG_LRN      0x00000200
#define MASK_LOG_POOLING  0x00000400
#define MASK_LOG_RELU     0x00000800
#define MASK_LOG_SIGMOID  0x00001000
#define MASK_LOG_SOFTMAX  0x00002000
#define MASK_LOG_TANH     0x00004000
#define MASK_LOG_LC       0x00008000
#define MASK_LOG_BN       0x00010000
#define MASK_LOG_CONCAT   0x00020000
#define APP_TIME_INFO     MASK_LOG_APP_TIME,"time:       \t"
#define ACL_ALLOCATE_INFO MASK_LOG_ALLOCATE,"allocate:   \t\t"
#define ACL_RUN_INFO      MASK_LOG_RUN,     "run:        \t\t\t"
#define ACL_CONFIG_INFO   MASK_LOG_CONFIG,  "configure:  \t\t\t\t"
#define ACL_COPY_INFO     MASK_LOG_COPY,    "tensor_copy:\t\t\t\t\t"
#define ACL_ABSVAL_INFO   MASK_LOG_ABSVAL,  "ACL_ABSVAL :\t\t\t\t\t\t"
#define ACL_SOFTRELU_INFO MASK_LOG_BNLL,    "ACL_BNLL   :\t\t\t\t\t\t\t"
#define ACL_CONV_INFO     MASK_LOG_CONV,    "ACL_CONV   :\t\t\t\t\t\t\t\t"
#define ACL_MATMUL_INFO   MASK_LOG_MATMUL,  "ACL_MATMUL :\t\t\t\t\t\t\t\t\t"
#define ACL_LRN_INFO      MASK_LOG_LRN,     "ACL_LRN    :\t\t\t\t\t\t\t\t\t\t"
#define ACL_POOLING_INFO  MASK_LOG_POOLING, "ACL_POOLING:\t\t\t\t\t\t\t\t\t\t\t"
#define ACL_RELU_INFO     MASK_LOG_RELU,    "ACL_RELU   :\t\t\t\t\t\t\t\t\t\t\t\t"
#define ACL_SIGMOID_INFO  MASK_LOG_SIGMOID, "ACL_SIGMOID:\t\t\t\t\t\t\t\t\t\t\t\t\t"
#define ACL_SOFTMAX_INFO  MASK_LOG_SOFTMAX, "ACL_SOFTMAX:\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
#define ACL_TANH_INFO     MASK_LOG_TANH,    "ACL_TANH   :\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
#define ACL_LC_INFO       MASK_LOG_LC,      "ACL_LC     :\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
#define ACL_BN_INFO       MASK_LOG_BN,      "ACL_BN     :\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
#define ACL_CONCAT_INFO   MASK_LOG_CONCAT,  "ACL_CONCAT :\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
extern int64_t acl_log_flags;

class logtime_util
{
  public:
    logtime_util(){
        mask =0;
    }
    logtime_util(int mask_, const char* information_){
      setlogtime_info(mask_,information_);
    }
    void setlogtime_info(int mask_, const char* information_){
      mask = mask_;
      if(acl_log_flags & mask){
        strncpy(information, information_, 255);
        gettimeofday(&tv[0], NULL);
      }
    }
    ~logtime_util(){
      if(acl_log_flags & mask){
        long time[2];
        gettimeofday(&tv[1], NULL);
        time[0] = tv[0].tv_sec * NANO_SEC_CONV + tv[0].tv_usec;
        time[1]   = tv[1].tv_sec * NANO_SEC_CONV + tv[1].tv_usec;
        printf("%s %.6lf\n", information, (((double)time[1] - time[0]) / NANO_SEC_CONV));
      }
    }
    void log_time(bool start)
    {
      if(acl_log_flags & mask){
        if (start){
          gettimeofday(&tv[0], NULL);
        }
        else{
          long time[2];
          gettimeofday(&tv[1], NULL);
          time[0] = tv[0].tv_sec * NANO_SEC_CONV + tv[0].tv_usec;
          time[1]   = tv[1].tv_sec * NANO_SEC_CONV + tv[1].tv_usec;
          printf("%s %.6lf\n", information, (((double)time[1] - time[0]) / NANO_SEC_CONV));
        }
      }
    }
private:
  struct timeval tv[2];
  int mask;
  char information[256];
};
#endif //USE_PROFILING

namespace tensorflow {

enum TensorType{
    tensor_input,
    tensor_output,
    tensor_weights,
    tensor_biases,
    tensor_data,
};

template <typename ACLTensor>
class BaseTensor:public ACLTensor{
public:
    BaseTensor(bool share)
       :share_(share),type_(tensor_input),allocate_(false){
    }
    virtual void bindmem(void *mem,bool share){
        mem_=mem;
        share_=share;
    }
    virtual void settensortype(TensorType type){
        type_=type;
    };
    virtual void map(bool blocking = true){}
    virtual void unmap(){}
    virtual void commit(TensorType type=tensor_data);
    int tensor_copy(void * mem, bool toTensor=true);
protected:
    void* mem_;
    bool share_;
    TensorType type_;
    bool allocate_;
};

class GPUTensor:public BaseTensor<arm_compute::CLTensor>{
public:
    explicit GPUTensor(bool share)
       :BaseTensor(share){}
    virtual void map(bool blocking = true){
        if (!allocate_){
            arm_compute::CLTensor::allocator()->allocate();
            allocate_=true;
        }
        arm_compute::CLTensor::map(blocking);
     }
     virtual void unmap(){
        arm_compute::CLTensor::unmap();
     }
};

class CPUTensor:public BaseTensor<arm_compute::Tensor>{
public:
    explicit CPUTensor(bool share)
        :BaseTensor(share){}
    virtual void map(bool blocking = true){
        if (!allocate_){
            arm_compute::Tensor::allocator()->allocate();
            allocate_=true;
        }
    }
    virtual void unmap(){
    }
};

template <typename ACLLayer,typename ACLTensor>
class ACLXPUBaseLayer{
public:
    virtual void commit(){
        if (input) {
            input->commit(tensor_input);
        }
        if (output){
            output->commit(tensor_output);
        }
        if (weights){
            weights->commit(tensor_weights);
        }
        if (biases){
            biases->commit(tensor_biases);
        }
    }
    
    virtual void run(bool gpu){
        commit();
        layer->run();
        if (gpu) {
            // Make sure all the OpenCL jobs are done executing:
            arm_compute::CLScheduler::get().sync();
        }
    }
    
    virtual bool reshape(arm_compute::TensorShape &shape,TensorType type);
    explicit ACLXPUBaseLayer(){
        layer=nullptr;
        input=nullptr;
        output=nullptr;
        weights=nullptr;
        biases=nullptr;
        mean=nullptr;
        var=nullptr;
        beta=nullptr;
        gamma=nullptr;
    }
    
    virtual void freelayer(){
        if (layer) delete layer;
        if (input) delete input;
        if (output) delete output;
        if (weights) delete weights;
        if (biases) delete biases;
        if (mean) delete mean;
        if (var) delete var;
        if (beta) delete beta;
        if (gamma) delete gamma;
        layer=nullptr;
        input=nullptr;
        output=nullptr;
        weights=nullptr;
        biases=nullptr;
        mean=nullptr;
        var=nullptr;
        beta=nullptr; 
        gamma=nullptr;
    }
 
    virtual ~ACLXPUBaseLayer(){
        freelayer();
    }
    ACLLayer *layer;
    ACLTensor *input;
    ACLTensor *output;
    ACLTensor *weights;
    ACLTensor *biases;
    //for BN
    ACLTensor *mean;
    ACLTensor *var;
    ACLTensor *beta; 
    ACLTensor *gamma;
};

template <typename GPULayer, typename CPULayer>
class ACLBaseLayer {
public:
    explicit ACLBaseLayer();
    virtual void gpu_run();
    virtual void cpu_run();
    virtual ~ACLBaseLayer();
    virtual GPULayer * new_gpulayer();
    virtual CPULayer * new_cpulayer();
    ACLXPUBaseLayer<GPULayer,GPUTensor>& gpu(){
        return gpu_;
    }
    ACLXPUBaseLayer<CPULayer,CPUTensor>& cpu(){
        return cpu_;
    }
    void checkreshape(arm_compute::TensorShape shape,bool gpu=false, TensorType type=tensor_input);
    void acl_run(void *input_data, void *output_data,bool gpu=false);
    template <typename ACLTensor> bool tensor_mem(ACLTensor *tensor,void *mem,bool share=false);
    template <typename ACLTensor> bool tensor_mem(void *mem,ACLTensor *tensor,bool share=false);
    template <typename ACLTensor> bool new_tensor(ACLTensor *&tensor,arm_compute::TensorShape shape,
						  void *mem=nullptr,bool share=false);
    virtual bool Bypass_acl(void* ctx);
protected:
    ACLXPUBaseLayer<GPULayer,GPUTensor> gpu_;
    ACLXPUBaseLayer<CPULayer,CPUTensor> cpu_;
    bool init_layer_;
    bool force_bypass_acl_path_;
};

}  // namespace tensorflow

#define acl_configure(xlayer, input, args...) \
            xlayer.layer->configure(input, args);

#define INSTANTIATE_ACLBASECLASS(GPULayer,CPULayer) \
  template class ACLBaseLayer<GPULayer,CPULayer>; 

#define INSTANTIATE_ACLBASE_FUNCTION(GPULayer,CPULayer,ACLTensor) \
    template bool ACLBaseLayer<GPULayer,CPULayer>::tensor_mem(ACLTensor *tensor,void *mem,bool share); \
    template bool ACLBaseLayer<GPULayer,CPULayer>::tensor_mem(void *mem,ACLTensor *tensor,bool share); \
    template bool ACLBaseLayer<GPULayer,CPULayer>::new_tensor(ACLTensor *&tensor,arm_compute::TensorShape shape,void *mem,bool share); \

#define is_gpu_  (std::is_same<Device, Eigen::GpuDevice>::value)


#if defined(TEST_ACL)

#define NHWC2NCHW(in, out, N, C, H, W)        \
  do {                                        \
    for (int n = 0; n < N; n++)               \
      for (int c = 0; c < C; c++)             \
        for (int h = 0; h < H; h++)           \
          for (int w = 0; w < W; w++)         \
            out[n*C*H*W + c*H*W + h*W + w] =  \
              in[n*H*W*C + h*W*C + w*C + c];  \
  } while(0)

#define NCHW2NHWC(in, out, N, C, H, W)        \
  do {                                        \
    for (int n = 0; n < N; n++)               \
      for (int c = 0; c < C; c++)             \
        for (int h = 0; h < H; h++)           \
          for (int w = 0; w < W; w++)         \
            out[n*H*W*C + h*W*C + w*C + c] =  \
              in[n*C*H*W + c*H*W + h*W + w];  \
  } while(0)

#define HWIO2OIHW(in, out, H, W, I, O)        \
  do {                                        \
    for (int h = 0; h < H; h++)               \
      for (int w = 0; w < W; w++)             \
        for (int i = 0; i < I; i++)           \
          for (int o = 0; o < O; o++)         \
            out[o*I*H*W + i*W*H + h*W + w] =  \
              in[h*W*I*O + w*I*O + i*O + o];  \
  } while(0)
#endif

#endif

#endif

