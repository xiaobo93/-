
# windowsServiceCode

windows服务程序应该编译为，windows控制台程序

1、windows创建服务程序的框架  
2、安装windows服务  
>**创建服务**  
sc create 服务名 binpath= "服务路径“//binpath= 和服务名之间要有一个空格  
启动服务  
sc start 服务名  
停止服务  
sc stop 服务名  
删除服务  
sc delete 服务名  

[参考链接](https://blog.csdn.net/blade1080/article/details/82015323)

其实调试C++ windows service和调试普通的C++ console App/MFC 没什么大的区别：

windows service跑起来之后，你可以选择Tools->Attach to Process 在Available Processes里面找到你要调试的服务，点击Attach即可。
如果你的服务是在Session 0 ,那么你可能要以管理员的身份打开VS,才能Attach.
一个服务只能被一个VS Attach。
请保证你的代码是最新的，就是说该代码就是就是生成该windows service的代码，否则windows service的执行结果就可能和你代码表现不一样。
在windows service最开始的代码片段可能无法调试(因为attach到service需要时间，但service跑起来却很快)。
  但是你可以在service开始阶段(比如初始化时)，加入Sleep(30).

原文链接：https://blog.csdn.net/lcl_data/article/details/7673043

******************************************  
  
# installService
>安装一个win32的服务程序  
服务安装之后，在注册表中的位置为：  
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services]  

  
********************************************


