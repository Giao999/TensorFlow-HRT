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
#if defined(USE_ACL)

#include "acl_ops_common.h"
#include "arm_neon.h"

namespace tensorflow {

static bool init_cl_env=true;
template <typename GPULayer, typename CPULayer>
ACLBaseLayer<GPULayer,CPULayer>::ACLBaseLayer()
    :init_layer_(true),force_bypass_acl_path_(false){
    if(init_cl_env){
        arm_compute::CLScheduler::get().default_init();
        init_cl_env=false;
    }
}
template <typename GPULayer, typename CPULayer>
void ACLBaseLayer<GPULayer,CPULayer>::gpu_run() {
    gpu_.run(true);
}
template <typename GPULayer, typename CPULayer>
void ACLBaseLayer<GPULayer,CPULayer>::cpu_run() {
    cpu_.run(false);
}

template <typename GPULayer, typename CPULayer>
ACLBaseLayer<GPULayer,CPULayer>::~ACLBaseLayer(){
}
template <typename GPULayer, typename CPULayer>
template <typename ACLTensor> bool ACLBaseLayer<GPULayer,CPULayer>::new_tensor(ACLTensor *&tensor,arm_compute::TensorShape shape,void *mem,bool share)
{
    tensor=new ACLTensor(share);
    tensor->allocator()->init(arm_compute::TensorInfo(shape, arm_compute::Format::F32));
    tensor->bindmem(mem,share);
    return true;
}

template <typename ACLTensor>
void BaseTensor<ACLTensor>::commit(TensorType type){
    settensortype(type);
    if (!share_&&mem_) {
        if (!allocate_){ 
            ACLTensor::allocator()->allocate(); 
            allocate_=true;
        }
        if (type_!= tensor_output) {
           tensor_copy(mem_);
        }
        mem_=nullptr;
    }
}

template <typename ACLTensor>
int BaseTensor<ACLTensor>::tensor_copy(void * mem,bool toTensor)
{
    arm_compute::Window window;
    ACLTensor* tensor=this;
    window.use_tensor_dimensions(tensor->info()->tensor_shape(), /* first_dimension =*/arm_compute::Window::DimY); // Iterate through the rows (not each element)
    int width = tensor->info()->tensor_shape()[0]; //->dimension(0); //window.x().end() - window.x().start(); // + 1;
    int height = tensor->info()->tensor_shape()[1]; //->dimension(1); //window.y().end() - window.y().start(); // + 1;
    int deepth = tensor->info()->tensor_shape()[2];
    map();
    // Create an iterator:
    arm_compute::Iterator it(tensor, window);
    // Except it works for an arbitrary number of dimensions
    if (toTensor) { //mem->tensor
        arm_compute::execute_window_loop(window, [&](const arm_compute::Coordinates & id)
        {
            memcpy(it.ptr(), ((char*)mem) + ((id[3] * (width * height * deepth) + id.z() * (width * height) + id.y() * width + id.x()) * tensor->info()->element_size()), width * tensor->info()->element_size());
        },
        it);
    }else{ //tensor-->mem
        arm_compute::execute_window_loop(window, [&](const arm_compute::Coordinates & id)
        {
            memcpy(((char*)mem) + ((id[3] * (width * height * deepth) + id.z() * (width * height) + id.y() * width) * tensor->info()->element_size()), it.ptr(), width * tensor->info()->element_size());
        },
        it);
    }
    unmap();

    return 0;
}

template <typename GPULayer, typename CPULayer>
template <typename ACLTensor> bool  ACLBaseLayer<GPULayer,CPULayer>::tensor_mem(ACLTensor *tensor,void *mem,bool share)
{
    tensor->bindmem(mem,share);
    return true;
}

template <typename GPULayer, typename CPULayer>
template <typename ACLTensor> bool  ACLBaseLayer<GPULayer,CPULayer>::tensor_mem(void *mem,ACLTensor *tensor,bool share)
{
    if (mem==tensor->buffer()) return true;
    if (!share) {
     tensor->tensor_copy(mem,false);
    }
    return true;
}
template <typename GPULayer, typename CPULayer>
void ACLBaseLayer<GPULayer,CPULayer>::acl_run(void *input_data, void *output_data,bool gpu)
{
    if (gpu) {
        if(input_data)tensor_mem(this->gpu().input,input_data);
        gpu_run();
        tensor_mem(output_data,this->gpu().output);
    }else{
        if(input_data)tensor_mem(this->cpu().input,input_data);
        cpu_run();
        tensor_mem(output_data,this->cpu().output);
    }
}


template <typename GPULayer, typename CPULayer>
bool ACLBaseLayer<GPULayer,CPULayer>::checkreshape(arm_compute::TensorShape shape,bool gpu, TensorType type)
{
    if (gpu) {
        init_layer_ = gpu_.reshape(shape,type);
    }else{
        init_layer_ = cpu_.reshape(shape,type);
    }
    return init_layer_;
}

template <typename GPULayer, typename CPULayer>
GPULayer * ACLBaseLayer<GPULayer,CPULayer>::new_gpulayer(){
        gpu_.layer= new GPULayer;
        return gpu_.layer;
}
template <typename GPULayer, typename CPULayer>
CPULayer * ACLBaseLayer<GPULayer,CPULayer>::new_cpulayer(){
        cpu_.layer= new CPULayer;
        return cpu_.layer;
}
template <typename ACLLayer,typename ACLTensor>
bool ACLXPUBaseLayer<ACLLayer,ACLTensor>::reshape(arm_compute::TensorShape &shape,TensorType type)
{
    arm_compute::TensorShape _shape;
    if (!layer) return true;
    switch (type) {
    case tensor_biases:
        _shape = biases->info()->tensor_shape();
        break;
    case tensor_weights:
        _shape = weights->info()->tensor_shape();
        break;
    case tensor_output:
        _shape = output->info()->tensor_shape();
        break;
    case tensor_input:
    default:
        _shape = input->info()->tensor_shape();
        break;
    }
    if (_shape.total_size()==shape.total_size() && _shape[0]==shape[0] && _shape[1]==shape[1]) {
        return false;
    }
    freelayer();
    return true;
}

INSTANTIATE_ACLBASECLASS(arm_compute::CLNormalizationLayer,arm_compute::NENormalizationLayer); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLNormalizationLayer,arm_compute::NENormalizationLayer,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLNormalizationLayer,arm_compute::NENormalizationLayer,CPUTensor);
INSTANTIATE_ACLBASECLASS(arm_compute::CLActivationLayer,arm_compute::NEActivationLayer); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLActivationLayer,arm_compute::NEActivationLayer,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLActivationLayer,arm_compute::NEActivationLayer,CPUTensor);
INSTANTIATE_ACLBASECLASS(arm_compute::CLPoolingLayer,arm_compute::NEPoolingLayer); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLPoolingLayer,arm_compute::NEPoolingLayer,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLPoolingLayer,arm_compute::NEPoolingLayer,CPUTensor);
INSTANTIATE_ACLBASECLASS(arm_compute::CLSoftmaxLayer,arm_compute::NESoftmaxLayer); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLSoftmaxLayer,arm_compute::NESoftmaxLayer,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLSoftmaxLayer,arm_compute::NESoftmaxLayer,CPUTensor);
INSTANTIATE_ACLBASECLASS(arm_compute::CLFullyConnectedLayer,arm_compute::NEFullyConnectedLayer); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLFullyConnectedLayer,arm_compute::NEFullyConnectedLayer,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLFullyConnectedLayer,arm_compute::NEFullyConnectedLayer,CPUTensor);
INSTANTIATE_ACLBASECLASS(arm_compute::CLConvolutionLayer,arm_compute::NEConvolutionLayer); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLConvolutionLayer,arm_compute::NEConvolutionLayer,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLConvolutionLayer,arm_compute::NEConvolutionLayer,CPUTensor);
INSTANTIATE_ACLBASECLASS(arm_compute::CLConvolutionLayer,arm_compute::NEDirectConvolutionLayer); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLConvolutionLayer,arm_compute::NEDirectConvolutionLayer,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLConvolutionLayer,arm_compute::NEDirectConvolutionLayer,CPUTensor);
INSTANTIATE_ACLBASECLASS(arm_compute::CLBatchNormalizationLayer,arm_compute::NEBatchNormalizationLayer); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLBatchNormalizationLayer,arm_compute::NEBatchNormalizationLayer,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLBatchNormalizationLayer,arm_compute::NEBatchNormalizationLayer,CPUTensor);
INSTANTIATE_ACLBASECLASS(arm_compute::CLLocallyConnectedLayer,arm_compute::NELocallyConnectedLayer); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLLocallyConnectedLayer,arm_compute::NELocallyConnectedLayer,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLLocallyConnectedLayer,arm_compute::NELocallyConnectedLayer,CPUTensor);
INSTANTIATE_ACLBASECLASS(arm_compute::CLDepthConcatenate,arm_compute::NEDepthConcatenate); 
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLDepthConcatenate,arm_compute::NEDepthConcatenate,GPUTensor);
  INSTANTIATE_ACLBASE_FUNCTION(arm_compute::CLDepthConcatenate,arm_compute::NEDepthConcatenate,CPUTensor);

}  // namespace tensorflow
#endif
