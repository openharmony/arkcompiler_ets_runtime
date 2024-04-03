# 运行方舟编译器工作负载

## 介绍

自动化运行js性能测试用例并且生成daily报告，用于测试方舟builtins API 性能。

特殊说明：下面文字中的"v 8" 、"v_8"、"d 8"、"d_8", 是为了规避门禁codecheck告警，请阅读及实际执行时去除或忽略中间的空格或”_“。

## 环境依赖

- 对于脚本所在的文件目录，当前用户需要拥有创建文件夹的权限。请注意当前目录的用户和用户组权限设置

- openpyxl。没有版本要求。若未安装请执行`pip3 install openpyxl`

- 环境中需要下载全仓鸿 蒙镜像或独立仓代码，并进行编译ark_js_vm 和 前端

  ```shell
  ./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ets_frontend_build # x86_64平台
  ```

- 配置v 8引擎执行环境

**下载：**

```shell
# 可以在任意目录。假定当前目录在/home目录
wget https://storage.googleapis.com/chromium-%76%38/official/canary/%76%38-linux64-rel-12.0.267.zip --no-check-certificate -O v_8-linux64-rel-12.0.267.zip
unzip v_8-linux64-rel-12.0.267.zip -d v_8
```

**配置：**

临时配置：

每次执行run_js_perf_test.sh脚本时传入 v 8 引擎 d 8 二进制的路径，比如/home/v_8/d 8。

永久配置：

```shell
sudo su
# 编辑/root/.bashrc文件在末尾追加以下内容
export PATH=/home/v_8:$PATH
# 刷新环境变量。注意执行sh脚本依然需要传入d 8的路径
source /root/.bashrc
```

**验证：**

```shell
执行： 
	/home/v_8/d 8 -v # 任意用户
回显：
	V 8 version 12.0.267
	d 8>
```

环境v 8引擎配置成功

## 文件目录

```bash
/arkcompiler/ets_runtime
│_ test                   # 模块测试用例
    |_ jsperftest         # js perf 测试代码目录
		 |-README.md      # 说明书
		 |-run_js_perf_test.sh # 测试执行shell脚本
		 |_run_js_test.py # 测试执行调用的python脚本
		 |_config.json    # 代码仓库与平台配置文件
```

**代码仓库与平台配置文件config.json说明：**

**文件内容：**

```shell
{
	"full_x86_64":{
		"ETS_RUNTIME_PATH": "out/rk3568/c lang_x64/arkcompiler/ets_runtime/",
		"ICU_PATH": "out/rk3568/c lang_x64/thirdparty/icu/",
		"ZLIB_PATH": "out/rk3568/c lang_x64/thirdparty/zlib/",
		"LIB_PATH": "prebuilts/c lang/ohos/linux-x86_64/l lvm/lib/",
		"ES2ABC": "out/rk3568/c lang_x64/arkcompiler/ets_frontend/es2abc"
	},
	"independent_x86_64": {
		"ETS_RUNTIME_PATH": "out/x64.release/arkcompiler/ets_runtime",
		"ICU_PATH": "out/x64.release/c lang_x64/thirdparty/icu/",
		"ZLIB_PATH": "",  	# 若无此配置，可不填，但不能删除该配置项
		"LIB_PATH": "",		# 若无此配置，可不填，但不能删除该配置项
		"ES2ABC": "out/x64.release/arkcompiler/ets_frontend/es2abc"
	}	
}
```

**说明：**

- 请无视“c lang"中间的空格，文件实际内容是无空格，这里是为了屏蔽门禁codecheck告警
- 在不同的代码仓库在不同的OS平台上，脚本运行需要的二进制的路径是不同的。脚本无法全覆盖。因此需要用户自己配置仓库信息和脚本执行平台信息
- full表示是全仓--鸿 蒙镜像下的代码仓；independent表示独立仓（区别于全仓）
- 脚本执行平台都是linux环境。默认全仓， full_x86_64。x86_64表示是x86 64位平台；aarch_64表示是arm 64位平台
- 请用户根据自己的环境需要在config.json中配置仓库与执行平台信息。并将json主键（比如full_x86_64）作为可选的第四参数传递给sh脚本

## 运行测试

1) 调用入口：bash run_js_perf_test.sh js_report_save_path openharmony_path d_8_binary_path ver_platform
   * js_report_save_path：必选]，表示报告输出的目录路径，该路径同时也是历史报告绝 对路径
   * openharmony_path:  必选]，将全仓鸿 蒙系统或者独立仓根目录（out的父目录）的绝 对路径
   * d_8_binary_path: [必选]，v 8 引擎 d 8 二进制的绝 对路径
   * ver_platform: [可选]，代码仓库与平台配置模式。不配置默认值“full_x86_64”，表示全仓 x86_64平台执行
   
3) 运行生成看护报告说明
    ```bash
    1. jsperftest生成的daily报告，excel格式
    	命名格式：js_perf_test_result_年月日.xlsx，比如js_perf_test_result_20231206.xlsx。
    2. 报告表格表头输出"用例名称"、"场景"、"执行状态"、"ark用例执行耗时(ms)"、"昨日ark用例执行耗时(ms)"、"是否劣化"、"v 8(ms)"、"v 8 --jitless(ms)"、"ark/v 8"、"ark/v 8 jitless"、"hyperlink"、"备注"；
       表格的后面，会追加汇总信息：用例数总数、执行成功数、失败数，劣化数目等内容。具体内容，请参见附录“daily报告”。
    3. 有执行失败时，当前用例的“执行状态”单元格内容会标记。
    4. “是否劣化”列，取值：true，false。当用例前一天性能数据不存在、js用例执行异常、执行失败，都归于没有劣化。
    5. 在daily报告生成目录，daily报告每日生成时会同时生成通知邮件汇总内容的文件--jsperftest_notifying_info_in_email.json。内容如下：
    	{
        	"kind": "V 8 js-perf-test",
        	"Total": 7,
        	"Ark劣化v 8": 1,
        	"Ark劣化v 8 jitless": 4
        }
    ```

## daily报告

请参见附录"Daily报告"。

## workload代码仓

  [Ark-workload](https://gitee.com/dov1s/arkjs-perf-test/tree/builtins_test1110/)

## 附录

### Daily报告

| 用例名称                                                | 场景               | 执行状态       | ark用例执行耗时(ms) | 是否劣化 | v 8(ms) | v 8 --jitless(ms) | ark/v 8              | ark/v 8 jitless    | hyperlink                                                    | 备注 |
| ------------------------------------------------------- | ------------------ | -------------- | ------------------- | -------- | ------ | ---------------- | ------------------- | ------------------ | ------------------------------------------------------------ | ---- |
| decodeuricomponent/decodeuricomponent.js                | decodeuricomponent | pass           | 42                  | NA       | 18     | 33               | 2.33 | 1.27 | https://gitee.com/dov1s/arkjs-perf-test/tree/builtins_test1110/js-perf-test/decodeuricomponent/decodeuricomponent.js |      |
| finalizationregistry/finalizationregistryconstructor.js | testconstructor    | pass           | 6                   | NA       | 10     | 26               | 0.6 | 0.23            | https://gitee.com/dov1s/arkjs-perf-test/tree/builtins_test1110/js-perf-test/finalizationregistry/finalizationregistryconstructor.js |      |
| finalizationregistry/register.js                        | testregister       | pass           | 16                  | NA       | 14     | 44               | 1.14        | 0.36            | https://gitee.com/dov1s/arkjs-perf-test/tree/builtins_test1110/js-perf-test/finalizationregistry/register.js |      |
| finalizationregistry/unregister.js                      | testunregister     | pass           | 20                  | NA       | 51     | 99               | 0.39            | 0.20           | https://gitee.com/dov1s/arkjs-perf-test/tree/builtins_test1110/js-perf-test/finalizationregistry/unregister.js |      |
| decodeuri/decodeuri.js                                  | decodeuri          | pass           | 46                  | NA       | 19     | 36               | 2.42 | 1.28 | https://gitee.com/dov1s/arkjs-perf-test/tree/builtins_test1110/js-perf-test/decodeuri/decodeuri.js |      |
|                                                         |                    |                |                     |          |        |                  |                     |                    |                                                              |      |
|                                                         |                    |                |                     |          |        |                  |                     |                    |                                                              |      |
| 劣化判定比率上限                                        |                    | 0.05           |                     |          |        |                  |                     |                    |                                                              |      |
| Totle js case count                                     |                    | 5           |                     |          |        |                  |                     |                    |                                                              |      |
| Pass count                                              |                    | 5           |                     |          |        |                  |                     |                    |                                                              |      |
| Fail count                                              |                    | 0              |                     |          |        |                  |                     |                    |                                                              |      |
| ark今日劣化数量                                         |                    | 2              |                     |          |        |                  |                     |                    |                                                              |      |
| Total excute time is(s)                                 |                    | 0:0:19.699970 |                     |          |        |                  |                     |                    |                                                              |      |
| ark/v 8 劣化数量                                    |  | 2             |                     |          |        |                  |                     |                    |                                                              |      |
| ark/v 8 jitless 劣化数量                             |                    | 0           |                     |          |        |                  |                     |                    |                                                              |      |

说明：

- E列“昨日用例执行耗时（ns)”默认是隐藏的。
- G列 “v 8(ms)”是有基准数据每月1、11、21日会生成v 8执行时间的基准数据，其它时间都不进行v 8执行用例操作，直接获取前边近一次的基准数据
- H列 “v 8 --jitless(ms)”是有基准数据每月1、11、21日会生成v 8 --jitless执行时间的基准数据，其它时间都不进行v 8 --jitless执行用例操作，直接获取前边近一次的v 8 --jitless基准数据
