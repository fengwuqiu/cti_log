### 使用方法
第一步：类似cit_msgs,将ctilog文件夹拷贝到你的工程目录/src/下面  
第二步：修改你的CMakeList.txt文件  
```
find_package(catkin REQUIRED COMPONENTS
  ctilog #加上这行
  roscpp
)
```
第三步：修改cpp文件，参考ctilog_example/src/example.cpp。加上头文件就可以直接使用了。  


### 备注
1. /cti/log/level话题设置日志等级
2. 日志输出两个文件*.log和*.log.1,其中*.log.1为缓存日志，*log为实时日志.
