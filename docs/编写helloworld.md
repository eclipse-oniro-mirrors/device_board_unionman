### 新建目录及源码
在源码根目录下vendor/unionpi/a311d目录下  
  
创建helloworld文件夹:

```
mkdir helloworld
```

进入helloworld文件夹中，创建helloworld.c文件:

```
#include <stdio.h>

void HelloPrint()
{
    printf("\n************************************************\n");
    printf("\n\t\tHello World! - UnionPi\n");//内容自定义
    printf("\n************************************************\n");
}

int main(int argc, char *argv[])
{
    HelloPrint();                                                                                            
    return 0;
}
```  

### 新建编译组织文件
同一个目录下vendor/unionpi/a311d/helloworld，创建BUILD.gn文件:
```
import("//build/ohos.gni")

ohos_executable("helloworld") {
  install_enable = true
  sources = [
    "helloworld.c",
  ]

  install_images = [ "system" ]
  part_name = "unionpi_products"
}
```

### 将新建源码纳入编译
修改device\unionpi\a311d路径下BUILD.gn文件，将新建的helloworld源码纳入编译体系，具体修改内容如下：
```
group("a311d_group") {
  deps = [
    "system:system",
    "kernel:kernel",
    "distributedhardware:distributedhardware",
    "bluetooth:bluetooth",
    "system_hap:hap",
    "//device/unionpi/hardware:hardware_group",
    "//device/unionpi/third_party:third_party",
    "//vendor/unionpi/a311d/helloworld:helloworld",//将新建的helloworld源码纳入编译体系
  ]
}
```
到此helloworld编写完成。