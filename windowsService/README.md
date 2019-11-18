
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

******************************************  
  
# installService
>安装一个win32的服务程序  
服务安装之后，在注册表中的位置为：  
[HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services]  

  
********************************************


