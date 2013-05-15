[2012/12/10]
主程序更新记录
-添加WIFI支持，需要在finsh里面执行 wlan() 来初始化WIFI。
 然后执行 wlan_begin("SSID", "passwd")即可关联到WIFI。
 1. WIFI使用需要准备根目录下的： /firmware 里面的固件。
 2. 无密码时执行 wlan_begin("SSID") 即可。

[2012/11/12]
主程序更新记录
-升级 RT-Thread 到最新开发分支
-将 SPI Flash 加载到文件系统根目录 “/”，将 SD 卡加载到 “/SD” 目录，应用程序默认搜索路径修改为 “/SD/programs”
-加入网络收音机移植，支持 ARMCC 和 IAR 编译器，暂不支持 GCC 编译器
-完善以太网，I2C, NandFlash 驱动
-修复文件系统重入问题
-修复应用模块名称过长导致的加载时系统崩溃问题

应用程序更新记录
-加入 snake 游戏应用程序
-更新部分应用程序图标

[2012/09/03]
主程序更新记录
-修正 programs 页面只能显示两个应用程序图标问题
-打开外扩 SRAM，解决因内存不足导致的系统稳定性问题
-使能 Lwip 组件

应用程序更新记录
-加入图标列表应用程序
-加入文件浏览应用程序

已知 bug:
-屏幕刷新过慢
-部分控件存在多次刷新问题

[2012/08/27]
下载地址： http://pan.baidu.com/share/link?shareid=8747&uk=4264488348

下载 RealTouch_20120827_release.zip, 解压后，总共有四个目录
bin -预先编译生成的 RealTouch 主程序，可以直接烧入 RealTouch 中运行
realtouch -RealTouch 源代码，可以直接使用 MDK 打开工程 project.uvproj 进行编译，生成最终目标文件; 也可以使用 arm-none-eabi-gcc 工具链进行编译
sdcard -需要将该目录下的文件夹拷贝到在 RealTouch 上使用的 SD 卡中；
programs -应用程序开发包

sdcard\programs -预先存放了三个应用程序 button，label， picture; RealTouch 启动时会扫描该目录，并加载应用程序图标
sdcard\picture -相册程序需要使用的图片

RealTouch 主程序正常运行后，首先会进入触摸屏校准程序，按照屏幕提示用触笔点击十字叉来完成校准过程；
如果校准后点击屏幕仍无效，则需要删除文件系统 /setup.ini 文件，才能重新进行开机自动校准。

更新记录
-修正刚上电时 SD 卡文件系统初始化失败问题
-修正 spiflash 文件系统挂载问题
-解决应用程序被多次加载问题
-加入将触屏校准信息写入文件的功能
-加入相册应用程序
-导出更多应用程序可使用的 API
-增加应用程序开发包

已知 bug :
-programs 页面只能显示两个应用程序图标

[2012/08/19]
这是第一个发布的 RealTouch 主程序，加入了 RealTouch UI，支持 SD 卡文件系统，并且支持动态加载应用程序;
整体上还比较粗糙，不过有了动态加载应用程序，大家可以参与进来开发啦，开发属于你自己的 UI 应用程序，
主程序争取以一至两周发布一个版本这样的频率来推进吧；

下载地址： http://pan.baidu.com/share/link?shareid=3822&uk=4264488348

下载 RealTouch_20120819_release.zip, 解压后，总共有三个目录
bin -预先编译生成的 RealTouch 主程序，可以直接烧入 RealTouch 中运行
realtouch -RealTouch 源代码，可以直接使用 MDK 打开工程 project.uvproj 进行编译，生成最终目标文件
sdcard -需要将该目录下的文件夹拷贝到在 RealTouch 上使用的 SD 卡中；

sdcard\programs -预先存放了两个应用程序 button 和 label; RealTouch 启动时会扫描该目录，并加载应用程序图标
sdcard\picture -相册程序需要使用的图片

RealTouch 主程序正常运行后，首先会进入触摸屏校准程序，按照屏幕提示用触笔点击十字叉来完成校准过程；
然后自动进入相册程序，循环播放 SD 卡中存放的相片；
如果触摸屏被正确校准，则点击屏幕左上角的彩色图标就会进入 UI 主程序；
UI 主程序分为三部分页面:
-programs 将 SD 卡中应用程序显示出来的图标列表页面，点击页面中的图标，就会运行相应的应用程序；
-Task 系统当前正在运行的应用程序列表页面，可以在该页面进行应用程序切换，以及应用程序关闭的操作；
-Setting 系统设置程序页面，暂时还没有相关的程序；

已知 bug : 
-每次刚上电时文件系统初始化会失败，需要按下扩展版上 reset 重启下，文件系统初始化才能成功；