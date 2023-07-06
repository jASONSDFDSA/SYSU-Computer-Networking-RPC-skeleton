# SYSU-Computer-Networking-RPC-skeleton
中山大学计算机网络实验课大作业RPC框架实现

在Linux中用gcc编译器按照registerCenter，server，client的顺序启动。cJSON是开源的C语言JSON序列化库，请查阅相关资料进行学习。client.c, server.c, registerCenter.c中用到的代码都在rpc.c中。rpc.h定义了各种结构体并且声明了函数，在每个函数前有注释解释函数功能。

以下是启动代码示例：

./registerCenter -l 127.0.0.1 -p 4001

./server -l 127.0.0.1 -p 3000 -l 127.0.0.1 -p 4001

./client -l 127.0.0.1 -p 4001
