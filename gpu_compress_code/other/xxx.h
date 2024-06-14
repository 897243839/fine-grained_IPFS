
// Benchmark performance from the binary data file fname

// 定义 CChar 类
#include <stdint.h>
#include <stdbool.h>
#define CChar unsigned char
uint8_t* compressed_cuda(uint8_t* input_buffer, const size_t input_size,size_t* size);
uint8_t* decomp_cuda(uint8_t* input_buffer, const size_t input_size,size_t* size);
uint8_t* cudamalloc(uint8_t* data);
uint8_t* decomp_cuda_cache(uint8_t* input_buffer, const size_t input_size,size_t* size,bool* flag);
uint8_t* Cacheblock(size_t* size);
int Push_cacheblock(uint8_t* input_buffer, const size_t input_size,bool* flag);
