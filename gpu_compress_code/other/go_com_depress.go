package main

// #cgo LDFLAGS: -L./ -lhigdecomcache
//#include "xxx.h"
/*
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"github.com/pierrec/lz4"
	"time"
	"unsafe"
	"github.com/klauspost/compress/zstd"
)
import (
	"bytes"
	"io"
	//"bufio"
	"io/ioutil"
	"os"
)
//func init() {
 //  C.cudamalloc()
//}
// lz4解/压缩

func Lz4_compress1(val []byte) (value []byte) {
	compressedData := make([]byte, lz4.CompressBlockBound(len(val)))
	n, err := lz4.CompressBlock(val, compressedData, nil)
	if err != nil {
		return val
	}
	return compressedData[:n]
}

func Lz4_decompress1(data []byte) (value []byte) {
	decompressedData := make([]byte, 10*len(data))
	n, err := lz4.UncompressBlock(data, decompressedData)
	if err != nil {
		return data
	}
	return decompressedData[:n]
}



func Cuda_compress(val []byte) (value []byte) {
//---------------------------压缩
	
	//var CudaLz4Header = []byte{0xf5,0x04,0x22}
	var size C.size_t
	//cByteStream := C.go_data_comp((*C.char)(unsafe.Pointer(&val[0])), C.int(len(val)), &size)
 // 调用C函数
  	cByteStream:=C.compressed_cuda((*C.uint8_t)(unsafe.Pointer(&val[0])), C.size_t(len(val)),&size)
  
	if size==0{
    		fmt.Println("cuda_lz4_comfail")	
    		return nil		
    	}
    	
	// Convert the C array to a Go slice
        defer  C.cudamalloc(cByteStream)
	//buf := C.GoBytes(unsafe.Pointer(cByteStream), C.int(size))
	buf := (*[1 << 30]byte)(unsafe.Pointer(cByteStream))[:size:size]
	//buf= append(CudaLz4Header[:], buf...)
	//dst:=make([]byte,len(buf))
	//copy(dst,buf)
	/*
	var dedata bytes.Buffer
	r := bytes.NewReader(buf)
	_, err := io.Copy(&dedata, r)
	if err != nil {
		//println("解压错误", err)
		return val
	}
	 //fmt.Println(len(data))
	return dedata.Bytes()
	*/
	return buf
}

func Cuda_decompress(data []byte) (value []byte) {
	//---------------------------解压
	
	var size C.size_t
	//val:=data
	//cByteStream := C.go_data_re((*C.char)(unsafe.Pointer(&data[0])), C.int(len(data)), &size)
	cByteStream :=C.decomp_cuda((*C.uint8_t)(unsafe.Pointer(&data[0])), C.size_t(len(data)),&size)
	if size==0{
    		fmt.Println("cuda_lz4_decomfail")	
    		return data	
    	}
    	
    	defer  C.cudamalloc(cByteStream)
	// Convert the C array to a Go slice
	//out := C.GoBytes(unsafe.Pointer(cByteStream), C.int(size))
	out := (*[1 << 30]byte)(unsafe.Pointer(cByteStream))[:size:size]
	//out := (*[1 << 30]byte)(unsafe.Pointer(cByteStream))[:size:size]
	/*
	var dedata bytes.Buffer
	r := bytes.NewReader(out)
	_, err := io.Copy(&dedata, r)
	if err != nil {
		//println("解压错误", err)
		return data
	}
	 //fmt.Println(len(data))
	return dedata.Bytes()
	*/
	//dst:=make([]byte,len(out))
	//copy(dst,out)
	//return dst
	return out
}

func Cuda_cache_decompress(data []byte) (value []byte) {
	//---------------------------解压
	var size C.size_t
	var flag C.bool=C.bool(false)
	//val:=data
	//cByteStream := C.go_data_re((*C.char)(unsafe.Pointer(&data[0])), C.int(len(data)), &size)
	cByteStream :=C.decomp_cuda_cache((*C.uint8_t)(unsafe.Pointer(&data[0])), C.size_t(len(data)),&size,&flag)
	if size==0{
    		fmt.Println("cuda_lz4_decomfail")
    		    	f:=bool(flag)
    		fmt.Println("flag=",flag)
    		fmt.Println("f=",f)	
    		return data	
    	}

    	//fmt.Println("flag=",flag)
    	
    	defer  C.cudamalloc(cByteStream)
	// Convert the C array to a Go slice
	out := C.GoBytes(unsafe.Pointer(cByteStream), C.int(size))
	//out := (*[1 << 30]byte)(unsafe.Pointer(cByteStream))[:size:size]
	//out := (*[1 << 30]byte)(unsafe.Pointer(cByteStream))[:size:size]
	return out
}
// lz4解压缩
/*
var buf bytes.Buffer
var lz4writer = lz4.NewWriter(nil)
var lz4reader = lz4.NewReader(nil)

func Lz4_compress(val []byte) (value []byte) {

	lz4writer.Reset(&buf)
	_, err := lz4writer.Write(val)
	if err != nil {
	return val
	}
	err = lz4writer.Close()
	if err != nil {
	return val
	}
	return buf.Bytes()
}

func Lz4_decompress(data []byte) (value []byte) {
	lz4reader.Reset(bytes.NewReader(data))
	value, err := io.ReadAll(lz4reader)
	if err != nil {
	//println("解压错误", err)
	return data 
	}
	return value
}
*/

func Zstd_compress1(val []byte) (value []byte) {

	var buf bytes.Buffer
	
	//writer, _ := zstd.NewWriter(&buf)
        writer, _ := zstd.NewWriter(&buf,zstd.WithEncoderLevel(zstd.SpeedBestCompression))
        //writer, _ := zstd.NewWriter(&buf,zstd.WithEncoderLevel(zstd.SpeedBetterCompression))
        //writer, _ := zstd.NewWriter(&buf,zstd.WithEncoderLevel(zstd.SpeedFastest))
	_, err := writer.Write(val)
	if err != nil {
		return val
	}
	err = writer.Close()
	if err != nil {
		return val
	}

	//fmt.Println("put------------")
	////	//fmt.Println(val)
	////	//fmt.Println(buf.Bytes())
	//fmt.Println(len(buf.Bytes()))
	//fmt.Println(len(val))
	//fmt.Println("put------------")
	//----------

	return buf.Bytes()
}

func Zstd_decompress1(data []byte) (value []byte) {
	//---------------------------解压
	b := bytes.NewReader(data)
	var out bytes.Buffer
	r, err := zstd.NewReader(b)
	if err != nil {
		return data
	}
	defer r.Close()
	_, err = io.Copy(&out, r)
	if err != nil {
		//println("解压错误", err)
		return data
	}
	return out.Bytes()
}

func Lz4_compress(val []byte) (value []byte) {
	var buf bytes.Buffer
	writer := lz4.NewWriter(&buf)
	_, err := writer.Write(val)

	if err != nil {
		return val
	}
	err = writer.Close()
	if err != nil {
		return val
	}
	return buf.Bytes()
}

func Lz4_decompress(data []byte) (value []byte) {
	//---------------------------解压
	b := bytes.NewReader(data)
	var out bytes.Buffer
	r := lz4.NewReader(b)
	_, err := io.Copy(&out, r)
	if err != nil {
		//println("解压错误", err)
		return data
	}

	return out.Bytes()
}

var zstdcom, _ = zstd.NewWriter(nil,zstd.WithEncoderLevel(zstd.SpeedBestCompression))
// Zstd解压缩
func Zstd_compress(data []byte) (value []byte) {
	  
	  val:= zstdcom.EncodeAll(data, nil)
	  return val
}

var zstdde, _ = zstd.NewReader(nil)

func Zstd_decompress(data []byte) (value []byte) {
	  
	  value, err := zstdde.DecodeAll(data, nil)
	  if err != nil {
	  //println("解压错误", err)
	  return data
	  }
	  return value
}
func GetCacheBlock(){
	var size C.size_t

	cByteStream :=C.Cacheblock(&size)
	if size==0{
    		fmt.Println("get_cacheblock_fail")	
    		return 
    	}
    	defer  C.cudamalloc(cByteStream)
	// Convert the C array to a Go slice
	//out := C.GoBytes(unsafe.Pointer(cByteStream), C.int(size))
	C.GoBytes(unsafe.Pointer(cByteStream), C.int(size))
	//println("获取到=",string(out[size-46:]))
}
func push(test []byte){
    keyhead:=[]byte("QmPmZpeGiqshia1Dd6hBWk4KtEvSQpDiZWY685fAAeX1wh")
    test=append(test[:], keyhead...)
    var flag C.bool=C.bool(false)
    C.Push_cacheblock((*C.uint8_t)(unsafe.Pointer(&test[0])), C.size_t(len(test)),&flag)
   // f:=bool(flag)
   // fmt.Println("flag=",flag,p)
     
}
func main() {
    var fileNames []string
    var n=3776
    // 解析命令行参数
    args := os.Args[1:]
    argc := len(args)
    if argc == 0 {
        fmt.Println("Must specify at least one file.")
        return
    }

    // 如果使用了 `-f` 参数，假设只有一个待处理文件
    if args[0] == "-f" {
        if argc == 1 {
            fmt.Println("Missing file name following '-f'")
            return
        } else if argc > 2 {
            fmt.Println("Unknown extra arguments with '-f'.")
            return
        }

        fileNames = append(fileNames, args[1])
    }
    // 读取文件
    fmt.Println(fileNames[0])
    fileContent, err := ioutil.ReadFile(fileNames[0])
    //fmt.Println(len(fileContent))
    lz4data, err := ioutil.ReadFile(fileNames[0])
    if err != nil {
        fmt.Println("读取文件错误：", err)
        return
    }
   
   	startTimex := time.Now()
	//data:=Cuda_ans_compress(fileContent)
	
	data:=Cuda_compress(fileContent)
	dst:=make([]byte,len(data))
	copy(dst,data)
	//*cid
	//bdata = hc.Compress(bdata, hc.Mode)
			//keyhead:=[]byte(k.String())
	keyhead:=[]byte("QmPmZpeGiqshia1Dd6hBWk4KtEvSQpDiZWY685fAAeX1wh")
	dst=append(dst[:], keyhead...)
	//dst=append(keyhead, dst...)
	//*/
	
	durx := time.Since(startTimex)
	fmt.Printf("压缩时间：%s,压缩后大小%d\n", durx,len(data))
	
        startTime := time.Now()
	for i:=0;i<n;i++ {
		//startTime := time.Now()

		Cuda_compress(fileContent)
		//dur := time.Since(startTime)
		//fmt.Printf("压缩时间：%s,压缩后大小%d\n", dur,len(data))
	}
	dur := time.Since(startTime)


  /*
    dstFile,err:=os.Create("com.data")
    defer dstFile.Close()
    d:=bytes.NewReader(data)
    w:=bufio.NewWriter(dstFile)
    io.Copy(w,d)
    fmt.Println("写入文件成功！")
    
    
    */
    
        Cuda_decompress(dst)
        fmt.Println("解压成功！")
    	startTime1:= time.Now()
	for i:=0;i<n;i++ {
         Cuda_cache_decompress(dst)
         
         //println("da:",string(da[163512:]))
	}
	dur1:= time.Since(startTime1)
	
	for i:=0;i<n;i++ {
             GetCacheBlock()
         //println("da:",string(da[163512:]))
	}

    
/*
     // 将数据写入文件
     fmt.Printf("%d",len(data))
     
     
     file1, err := os.OpenFile("./yjybit.data", os.O_CREATE|os.O_WRONLY, 0644)
    if err != nil {
        fmt.Println("打开文件错误：", err)
        return
    }


    // 写入文件
    _, err = file1.Write(data)
    if err != nil {
        fmt.Println("写入文件错误：", err)
        return
    }

    // 关闭文件
    err = file1.Close()
    if err != nil {
        fmt.Println("关闭文件错误：", err)
        return
    }

    fmt.Println("写入文件成功！")
 */  
 
 fmt.Printf("总%d次cuda压缩时间：%s,压缩后大小%d\n", n,dur,len(data))
 fmt.Printf("总%d次cuda解压时间：%s\n",n, dur1)
   
   
    lz4com:=Zstd_compress(lz4data)
     
    startTime31 := time.Now()
    for i:=0;i<n;i++ {  
    lz4com=Zstd_compress(lz4data)
    }
    dur31 := time.Since(startTime31)
    fmt.Printf("总%d次zstdcpu无状态压缩时间：%s,压缩后大小%d\n",n, dur31,len(lz4com))
      startTime41 := time.Now()
    for i:=0;i<n;i++ {


    Zstd_decompress(lz4com)
    }
    dur41:= time.Since(startTime41)
    fmt.Printf("总%d次zstdcpu无状态解压时间：%s\n",n,dur41)
    
lz4com=Zstd_compress1(lz4data)

 startTime31 = time.Now()
    for i:=0;i<n;i++ {  
    lz4com=Zstd_compress1(lz4data)
    }
    dur31 = time.Since(startTime31)
    fmt.Printf("总%d次zstdcpu压缩时间：%s,压缩后大小%d\n",n, dur31,len(lz4com))


     startTime44 := time.Now()
      
    for i:=0;i<n;i++ {
    Zstd_decompress1(lz4com)
    }
    dur44:= time.Since(startTime44)
    fmt.Printf("总%d次zstdcpu解压时间：%s\n",n,dur44)    
    
  
    lz4com=Lz4_compress(lz4data)
    // fmt.Println(len(lz4com))
     startTime3:= time.Now()
    for i:=0;i<n;i++ { 
    lz4com=Lz4_compress(lz4data)
  }
    dur3:= time.Since(startTime3)
    fmt.Printf("总%d次lz4cpu压缩时间：%s,压缩后大小%d\n",n, dur3,len(lz4com))
    startTime4 := time.Now()
    for i:=0;i<n;i++ {
    
    Lz4_decompress(lz4com)
    push(lz4com)
    
    }
    dur4:= time.Since(startTime4)
    fmt.Printf("总%d次lz4cpu解压时间：%s\n",n, dur4)

	for i:=0;i<n;i++ {
             GetCacheBlock()
         //println("da:",string(da[163512:]))
	}

    lz4com=Lz4_compress1(lz4data)
     //fmt.Println(len(lz4com))
     startTime33:= time.Now()
    for i:=0;i<n;i++ {  
    lz4com=Lz4_compress1(lz4data)
    }
    dur33:= time.Since(startTime33)
    fmt.Printf("总%d次lz4cpu,无状态压缩时间：%s,压缩后大小%d\n",n, dur33,len(lz4com))
     startTime45:= time.Now()
    for i:=0;i<n;i++ {
   
    Lz4_decompress1(lz4com)
  //  fmt.Printf("???")
    push(lz4com)
    
    }
    dur45 := time.Since(startTime45)
    	for i:=0;i<n;i++ {
             GetCacheBlock()
         //println("da:",string(da[163512:]))
	}
    fmt.Printf("总%d次lz4cpu，无状态解压时间：%s\n",n, dur45)
    
}
   
