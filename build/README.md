# ark_build
注意：目前版本部分内容还需完善，后续入库后会做具体说明

#### 介绍
方舟独立编译构建build

#### 软件架构和目录
独立编译目录结构

/arkcompiler/ 

├── ets_runtime 

    ├── build     
           
├── runtime_core   
          
├── ets_frontend         

├── third_patry         #从openharmony开源项目下载


#### 安装教程
1.  独立编译拉取arkcompiler_ets_runtime、arkcompiler_runtime_core、arkcompiler_ets_frontend三个仓（拉取时注意去掉前缀arkcompiler），执行[./ets_runtime/build/prebuilts_download.sh] 下载相关的编译所需工具。
2.  之后执行[./ets_runtime/build/compile_script/gen.sh ark]将.gn、.sh文件拿出来。
3.  执行[./gen.sh]命令编译目前独立编译支持的所有目标，执行[./gen.sh abc]命令生成abc文件，执行[./gen.sh .]命令执行abc文件

#### 使用说明

1.  独立编译build目录放在ets_runtime仓下，它和build仓不一样，是独立编译特有的
2.  所需三方库从openharmony开源项目下载，包含 :
        https://gitee.com/openharmony/third_party_bounds_checking_function,
        https://gitee.com/openharmony/third_party_icu,
        https://gitee.com/openharmony/third_party_jsoncpp,
        https://gitee.com/openharmony/third_party_zlib
3.  编译时需拉取三方库到arkcompiler/third_party/xxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request
