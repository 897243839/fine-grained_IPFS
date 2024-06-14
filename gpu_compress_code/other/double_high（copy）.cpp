/*
 * Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <random>
#include <assert.h>
#include <iostream>
#include <cstring>

#include <fstream>
#include "nvcomp/ans.hpp"
#include "nvcomp/lz4.hpp"
#include "nvcomp/cascaded.hpp"
#include "nvcomp/bitcomp.hpp"
#include "nvcomp.hpp"
#include "nvcomp/nvcompManagerFactory.hpp"
#include <cuda_runtime.h>
#include "nvcomp/gdeflate.hpp"
#include "nvcomp/deflate.hpp"
#include "nvcomp/zstd.hpp"
#include <cuda_runtime_api.h>
#include <list>
#include <algorithm>
#include <mutex>
#include <queue>



/* 
  To build, execute
  
  mkdir build
  cd build
  cmake -DBUILD_EXAMPLES=ON ..
  make -j

  To execute, 
  bin/high_level_quickstart_example
*/

using namespace nvcomp;

#define CUDA_CHECK(cond)                                                       \
  do {                                                                         \
    cudaError_t err = cond;                                                    \
    if (err != cudaSuccess) {                                               \
      std::cerr << "Failure" << std::endl;                                \
                                                                    \
    }                                                                         \
  } while (false)
  

/**
 * In this example, we:
 *  1) compress the input data
 *  2) construct a new manager using the input data for demonstration purposes
 *  3) decompress the input data
 */ 
//uint8_t* device_input_ptrs;
  cudaStream_t stream;
// Memory pool class


class MemoryPoolHost {
public:
    MemoryPoolHost(size_t blockSize, size_t numBlocks) : blockSize(blockSize), numBlocks(numBlocks) {
   

        // Initialize the free list
        for (size_t i = 0; i < numBlocks; i++) {
             // Allocate memory for the pool
            cudaError_t err = cudaMallocHost(&pool, blockSize);
            if (err != cudaSuccess) {
            std::cerr << "Error allocating memory for pool: " << cudaGetErrorString(err) << std::endl;
            exit(1);
            }

            freeList.push_back(pool);

        }
    }

    ~MemoryPoolHost() {
        // Free the memory for the pool
        cudaFreeHost(pool);
    }

    void* getMemory(size_t size) {
            std::lock_guard<std::mutex> lock(mutex);
        // Check if the requested size is larger than the block size
        if (size > blockSize) {
            std::cerr << "Error: requested size is larger than block size" <<size<< std::endl;
            exit(1);
        }

        // Get a block from the free list
        
        if (freeList.empty()) {
              std::cerr << "Error: pool is out of memory" << std::endl;
        }
        
        void* block = freeList.back();
        freeList.pop_back();
        return block;
    }

    void returnMemory(void* block) {
        // Return the block to the free list
        std::lock_guard<std::mutex> lock(mutex);
        freeList.push_back(block);

    }

private:
    size_t blockSize;
    size_t numBlocks;
    void* pool;
    std::vector<void*> freeList;
    std::mutex mutex;
};

MemoryPoolHost pool(762158,50);

typedef struct{
	void* block;
	size_t size;
}CacheBlock;

class BlockCachePool {
public:
    BlockCachePool(size_t blockSize, size_t numBlocks) : blockSize(blockSize), numBlocks(numBlocks) {
      CUDA_CHECK(cudaStreamCreate(&stream));
		/*
        // Initialize the free list
        for (size_t i = 0; i < numBlocks; i++) {
             // Allocate memory for the pool
            cudaError_t err = cudaMallocAsync(&pool, blockSize,stream);
            if (err != cudaSuccess) {
            std::cerr << "Error allocating memory for pool: " << cudaGetErrorString(err) << std::endl;
            exit(1);
            }
            freeList.push_back(pool);
        }
        */
    }

    ~BlockCachePool() {
        // Free the memory for the pool
        cudaFreeAsync(pool,stream);
        //cudaFree(pool);
    }
    
    CacheBlock* getCacheblock() {
         std::lock_guard<std::mutex> lock(mutex);
    
        // Get a block from the free list
        if (unfreeList.empty()) {
            std::cerr << "Error: pool is out of memory" << std::endl;
            return nullptr;
        }
        CacheBlock* block=new CacheBlock;
        block = (CacheBlock*)unfreeList.front();
        unfreeList.pop();
         // Get a block from the free list
        //if (freeList.empty()) {
        //    std::cerr << "Error: pool is out of memory" << std::endl;
        //}
        //void* block = freeList.back();
        //freeList.pop_back();
        //*size=block->size;
        return block;
    }

    bool Cacheblockadd(void* block,size_t size) {
    	if (unfreeList.size()>=numBlocks){
    		return false;
    	}
        CacheBlock* cacheblock=new CacheBlock;
        cacheblock->block=block;
        cacheblock->size=size;
        
        std::lock_guard<std::mutex> lock(mutex);
        // Return the block to the free list
        //freeList.push_back(block);
        unfreeList.push(cacheblock);
        return true;
       
    }

private:
    size_t blockSize;
    size_t numBlocks;
    void* pool;
    std::queue<void*> unfreeList;
    //std::vector<void*> freeList;
    std::mutex mutex;
};
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//block大小设定（暂留无效），与缓冲池最大块数量设定
BlockCachePool pool1(262158,7552);

 uint8_t* compressed_with_manager_factory(uint8_t* device_input_ptrs, const size_t input_buffer_len,size_t* size)
{
  cudaStream_t stream1;
  CUDA_CHECK(cudaStreamCreate(&stream1));
  const int chunk_size = 1 << 16;
  nvcompType_t data_type = NVCOMP_TYPE_CHAR;
  //ZstdManager nvcomp_manager{chunk_size,nvcompBatchedZstdDefaultOpts,stream, 0};
 // ZstdManager nvcomp_manager{chunk_size,{1},stream, 0};
 // GzipManager nvcomp_manager{chunk_size,nvcompBatchedGzipDefaultOpts,stream, 0};
  //BitcompManager nvcomp_manager{chunk_size, nvcompBatchedBitcompDefaultOpts, stream, 0};
  //GdeflateManager  nvcomp_manager(chunk_size,{1}, stream,0);
  //DeflateManager  nvcomp_manager(chunk_size,nvcompBatchedDeflateDefaultOpts, stream,0);
    //DeflateManager  nvcomp_manager(chunk_size,{2}, stream,0);
  ANSManager nvcomp_manager{chunk_size,nvcompBatchedANSDefaultOpts,stream, 0};
 // CascadedManager nvcomp_manager{chunk_size,nvcompBatchedCascadedDefaultOpts,stream, 0};
  //LZ4Manager nvcomp_manager{chunk_size,nvcompBatchedLZ4DefaultOpts,stream, 0};
  CompressionConfig comp_config = nvcomp_manager.configure_compression(input_buffer_len);
 // std::cout << "bitcomp" <<comp_config.max_compressed_buffer_size <<std::endl;

   uint8_t* comp_buffer = (uint8_t*)pool.getMemory(comp_config.max_compressed_buffer_size);    
  nvcomp_manager.compress(device_input_ptrs, comp_buffer, comp_config);
  //-------------------------------------------------------------------------------
// Copy compressed sizes from device to host
size_t compressd_size=nvcomp_manager.get_compressed_output_size(comp_buffer);
  //std::cout << "ANSCOM" << std::endl;
/*
std::vector<uint8_t> host_compressed_data(compressd_size);
//CUDA_CHECK(cudaMallocHost(&host_compressed_data, sizeof(size_t) *comp_config.num_chunks));
cudaMemcpyAsync(
    host_compressed_data.data(), comp_buffer,
    compressd_size, cudaMemcpyDeviceToHost, stream);
 */   
// Allocate memory for the C-style array
 // uint8_t* c_byte_stream = new uint8_t[compressd_size];

  // Copy the data from the vector to the C-style array
 // std::copy(host_compressed_data.begin(), host_compressed_data.end(), c_byte_stream);
 // std::memcpy(c_byte_stream, comp_buffer, compressd_size);
  // Set the size of the array
  *size = compressd_size;

 // pool.returnMemory(comp_buffer);
 // CUDA_CHECK(cudaFree(comp_buffer));

  CUDA_CHECK(cudaStreamSynchronize(stream1));

  CUDA_CHECK(cudaStreamDestroy(stream1));
  return comp_buffer;
}


uint8_t* decomp_with_manager_factory(uint8_t* device_input_ptrs, const size_t input_buffer_len,size_t* size)
{
	
  cudaStream_t stream2;
  
  CUDA_CHECK(cudaStreamCreate(&stream2));
        
 // std::cout << "bitcompdecom" << std::endl;
  /*
  const int chunk_size = 1 << 16;
  nvcompType_t data_type = NVCOMP_TYPE_CHAR;

  LZ4Manager nvcomp_manager{chunk_size, data_type, stream};
  CompressionConfig comp_config = nvcomp_manager.configure_compression(input_buffer_len);

  uint8_t* comp_buffer;
  CUDA_CHECK(cudaMalloc(&comp_buffer, comp_config.max_compressed_buffer_size));
  
  nvcomp_manager.compress(device_input_ptrs, comp_buffer, comp_config);
  */
  //-------------------------------------------------------------------------------
  //std::cout << "ANSDE" << std::endl;
  // Construct a new nvcomp manager from the compressed buffer.
  // Note we could use the nvcomp_manager from above, but here we demonstrate how to create a manager 
  // for the use case where a buffer is received and the user doesn't know how it was compressed
  // Also note, creating the manager in this way synchronizes the stream, as the compressed buffer must be read to 
  // construct the manager

  
  auto decomp_nvcomp_manager = create_manager(device_input_ptrs, stream);
  
  if(!decomp_nvcomp_manager){
    *size=0;
     return nullptr;
  }

  DecompressionConfig decomp_config= decomp_nvcomp_manager->configure_decompression(device_input_ptrs);

  //uint8_t* res_decomp_buffer;
 // CUDA_CHECK(cudaMalloc(&res_decomp_buffer, decomp_config.decomp_data_size));
  //cudaMallocAsync(reinterpret_cast<void**>(&res_decomp_buffer), decomp_config.decomp_data_size,stream);
   uint8_t* res_decomp_buffer = (uint8_t*)pool.getMemory(decomp_config.decomp_data_size);    
  decomp_nvcomp_manager->decompress(res_decomp_buffer, device_input_ptrs, decomp_config);
  //----------------------------------------------------------------------------------
  
/*
 std::vector<uint8_t> host_decompressed_data(decomp_config.decomp_data_size);
cudaMemcpyAsync(
    host_decompressed_pushCacheblockdata.data(), res_decomp_buffer,
    decomp_config.decomp_data_size, cudaMemcpyDeviceToHost, stream);
 */
 /*
std::ofstream decomp_file("decompressed_data.txt", std::ios::out | std::ios::binary);
decomp_file.write(reinterpret_cast<const char*>(host_decompressed_data.data()), decomp_config.decomp_data_size);
decomp_file.close();
  */
  // Allocate memory for the C-style array
  //uint8_t* c_byte_stream = new uint8_t[decomp_config.decomp_data_size];

  // Copy the data from the vector to the C-style array
 // std::copy(host_decompressed_data.begin(), host_decompressed_data.end(), c_byte_stream);
  //std::memcpy(c_byte_stream, res_decomp_buffer, decomp_config.decomp_data_size);
  // Set the size of the array
  //*size = host_decompressed_data.size();
  *size=decomp_config.decomp_data_size;

  //CUDA_CHECK(cudaFree(res_decomp_buffer));
  //pool.returnMemory(res_decomp_buffer);

  CUDA_CHECK(cudaStreamSynchronize(stream2));
  CUDA_CHECK(cudaStreamDestroy(stream2));
    return res_decomp_buffer;

}




extern "C" {

int Push_cacheblock(uint8_t* input_buffer, const size_t input_size,bool* flag){
//解压成功   将压缩块存入缓冲池
  uint8_t* de_input_ptrs;
  cudaError_t err =cudaMalloc(&de_input_ptrs, input_size);
   if (err != cudaSuccess) {                                               
      std::cerr << "Failure" << std::endl;        
      *flag=false;   
      return 0;                     
      } 
  //CUDA_CHECK(cudaMallocManaged(&device_input_ptrs, input_size));
   // cudaMemcpyAsync(de_input_ptrs, input_buffer, input_size, cudaMemcpyDefault,stream);
    cudaMemcpy(de_input_ptrs, input_buffer, input_size, cudaMemcpyDefault);
    //std::cout << "input_size=" <<input_size<< std::endl;      
    //std::cout << "Q=" <<*(input_buffer+(input_size-46))<< std::endl;
    //std::cout << "m=" <<*(input_buffer+(input_size-45))<< std::endl;                                 
   if(*(input_buffer+(input_size-46))!='Q'&&*(input_buffer+(input_size-45))!='m'){
   //解压失败   丢弃压缩块，不存入缓冲池  or 不是46位Qm开头cid,不存入缓冲池
     cudaFree(de_input_ptrs);
     *flag=false;	
   }else {
      //解压成功   将压缩块存入缓冲池
      *flag=pool1.Cacheblockadd(de_input_ptrs,input_size);
   }
   return 1;
}
uint8_t* Cacheblock(size_t* size){
     CacheBlock* cacheblock =pool1.getCacheblock();
     if (cacheblock==nullptr){
     *size=0;
     return nullptr;
     }
     *size=cacheblock->size;
     uint8_t* cudablock=(uint8_t*)cacheblock->block;
     //uint8_t* cacheblock= (uint8_t*)pool1.getCacheblock(size);
     uint8_t* cacheblockhost = (uint8_t*)pool.getMemory(*size);
     cudaMemcpy(cacheblockhost, cudablock,*size, cudaMemcpyDeviceToHost);
     cudaFreeAsync(cudablock,stream);	
     delete cacheblock;
    return cacheblockhost;
  }
uint8_t* cudamalloc(uint8_t* data){

   // cudaMallocAsync(&device_input_ptrs, 262158,stream);
   // cudaMallocAsync(&de_input_ptrs, 218392,stream);
     // cudaMallocManaged(&res_decomp_buffer, 562158);
     // cudaMemPrefetchAsync(res_decomp_buffer,562158,0);
     //cudaMallocHost(&res_decomp_buffer, 562158);
   pool.returnMemory(data);
   //uint8_t* gx;   
    //cudaMallocHost(&gx, 56215);
    //res_decomp_buffer=gx;
    //cudaMallocHost(&comp_buffer, 262158);
    return nullptr;
  }

uint8_t* compressed_cuda(uint8_t* input_buffer,const size_t input_size,size_t* size)
{
// 调用压缩函数
  //uint8_t* device_input_ptrs= (uint8_t*)pool1.getMemory(input_size);
   uint8_t* device_input_ptrs;
  cudaMallocAsync(&device_input_ptrs, input_size,stream);
 // const size_t inputsize = 262158; // 替换为实际的输入大小
  //cudaMalloc(&device_input_ptrs, 262158);
  //CUDA_CHECK(cudaMallocManaged(&device_input_ptrs, input_size));
  CUDA_CHECK(cudaMemcpyAsync(device_input_ptrs, input_buffer, input_size, cudaMemcpyDefault,stream));
  
  // Four roundtrip examples
  //decomp_compressed_with_manager_factory_example(device_input_ptrs, input_size);
  uint8_t* data=compressed_with_manager_factory(device_input_ptrs, input_size,size);
  //decomp_with_manager_factory(device_input_ptrs, input_size);
  //CUDA_CHECK(cudaFree(device_input_ptrs));
   //pool1.returnMemory(device_input_ptrs);
  cudaFreeAsync(device_input_ptrs,stream);
  return data;
}

uint8_t* decomp_cuda_sim(uint8_t* input_buffer, const size_t input_size,size_t* size)
{
// 调用解压函数  压缩文件无cid填充。
  //uint8_t* de_input_ptrs= (uint8_t*)pool1.getMemory(262158);
 uint8_t* de_input_ptrs;
   cudaMallocAsync(&de_input_ptrs, input_size,stream);
  //CUDA_CHECK(cudaMallocManaged(&device_input_ptrs, input_size));
  CUDA_CHECK(cudaMemcpyAsync(de_input_ptrs, input_buffer, input_size, cudaMemcpyDefault,stream));
  // Four roundtrip examples
 uint8_t* data=decomp_with_manager_factory(de_input_ptrs, input_size,size);
   //pool1.returnMemory(de_input_ptrs);
 cudaFreeAsync(de_input_ptrs,stream);	
  return data;
}



uint8_t* decomp_cuda(uint8_t* input_buffer, const size_t input_size,size_t* size)
{
// 调用解压函数  压缩文件有46位cid填充
  //uint8_t* de_input_ptrs= (uint8_t*)pool1.getMemory(262158);
 uint8_t* de_input_ptrs;
   cudaMallocAsync(&de_input_ptrs, input_size,stream);
  //CUDA_CHECK(cudaMallocManaged(&device_input_ptrs, input_size));
  CUDA_CHECK(cudaMemcpyAsync(de_input_ptrs, input_buffer, input_size, cudaMemcpyDefault,stream));
  const size_t input_size1=input_size-46; //解压时去除46位cid
  // Four roundtrip examples
 uint8_t* data=decomp_with_manager_factory(de_input_ptrs, input_size1,size);
   //pool1.returnMemory(de_input_ptrs);
 cudaFreeAsync(de_input_ptrs,stream);	
  return data;
}
uint8_t* decomp_cuda_cache(uint8_t* input_buffer, const size_t input_size,size_t* size,bool* flag)
{
// 调用解压函数  压缩文件有46位cid填充  使用gpu缓冲池
  //uint8_t* de_input_ptrs= (uint8_t*)pool1.getMemory(262158);
 uint8_t* de_input_ptrs;
   cudaMallocAsync(&de_input_ptrs, input_size,stream);
  //CUDA_CHECK(cudaMallocManaged(&device_input_ptrs, input_size));
  CUDA_CHECK(cudaMemcpyAsync(de_input_ptrs, input_buffer, input_size, cudaMemcpyDefault,stream));
  //const size_t input_size1=input_size;
  // Four roundtrip examples
 uint8_t* data=decomp_with_manager_factory(de_input_ptrs, input_size,size);
// std::cout << "input_buffer=" <<*(input_buffer+(input_size-46))<< std::endl;
//  std::cout << "input_buffer=" <<*(input_buffer+(input_size-45))<< std::endl;
   if(*size==0||(*(input_buffer+(input_size-46))!='Q'&&*(input_buffer+(input_size-45))!='m')){
   //解压失败   丢弃压缩块，不存入缓冲池  or 不是46位Qm开头cid,不存入缓冲池
     cudaFreeAsync(de_input_ptrs,stream);
     *flag=false;	
   }else {
      //解压成功   将压缩块存入缓冲池
      *flag=pool1.Cacheblockadd(de_input_ptrs,input_size);
   }
  return data;
}
}
/*
int main(int argc, char* argv[]){

  std::vector<std::string> file_names(argc - 1);

  if (argc == 1) {
    std::cerr << "Must specify at least one file." << std::endl;
    return 1;
  }

  // if `-f` is specified, assume single file mode
  if (strcmp(argv[1], "-f") == 0) {
    if (argc == 2) {
      std::cerr << "Missing file name following '-f'" << std::endl;
      return 1;
    } else if (argc > 3) {
      std::cerr << "Unknown extra arguments with '-f'." << std::endl;
      return 1;
    }

    file_names = {argv[2]};
  } else {
    // multi-file mode
    for (int i = 1; i < argc; ++i) {
      file_names[i - 1] = argv[i];
    }
  }



    std::ifstream inpupushCacheblockt_file(file_names[0], std::ios::in | std::ios::binary);
    if (!input_file) {
        std::cerr << "Failed to open input file!\n";
        return 1;
    }

    // 获取文件大小
    input_file.seekg(0, std::ios::end);
    size_t input_size = input_file.tellg();

    // 将文件指针回到开头
    input_file.seekg(0);

    // 从文件读取数据
    uint8_t* input_buffer = new uint8_t[input_size];
    input_file.read(reinterpret_cast<char*>(input_buffer), input_size);
    input_file.close();

    //cudamalloc();

    // 调用压缩函数
    uint8_t* device_input_ptrs;
  CUDA_CHECK(cudaMalloc(&device_input_ptrs, input_size));
  CUDA_CHECK(cudaMemcpy(device_input_ptrs, input_buffer, input_size, cudaMemcpyDefault));
  
    cudaEvent_t start,stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);
  cudaEventRecord(start);
  // Four roundtrip examples
  //decomp_compressed_with_manager_factory_example(device_input_ptrs, input_size);
  //compressed_with_manager_factory(device_input_ptrs, input_size);
  size_t* s; 
  for (int i=0;i<100000;i++){
  pool.returnMemory(decomp_with_manager_factory(device_input_ptrs, input_size,s));
  }
  CUDA_CHECK(cudaFree(device_input_ptrs));
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float elapsedTime;
    cudaEventElapsedTime(&elapsedTime, start, stop);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
     std::cout << "\n  time " << elapsedTime << " ms" << std::endl;  
    
    

    delete[] input_buffer;

    return 0;
}
*/

