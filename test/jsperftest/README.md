# 运行方舟编译器工作负载

## 介绍

自动化运行js性能测试用例并且生成daily报告，用于测试方舟builtins API 性能。

## 目录

```bash
/arkcompiler/ets_runtime
│_ test                   # 模块测试用例
    |_ jsperftest         # js perf 测试代码目录
		 |-README.md      # 说明书
		 |-run_js_perf_test.sh # 测试执行shell脚本
		 |_run_js_test.py # 测试执行调用的python脚本
```

## 运行测试
1) 调用入口：bash run_js_perf_test.sh js_report_save_path openharmony_path
   * js_report_save_path：表示报告输出的目录路径，该路径同时也是历史报告路径
   * openharmony_path:  将鸿蒙系统根目录（out的父目录）的路径
   
3) 运行生成看护报告说明
    ```bash
    1. jsperftest生成的daily报告，excel格式
    	命名格式：js_perf_test_result_年月日.xlsx，比如js_perf_test_result_20231206.xlsx。
    2. 报告表格表头输出"用例名称"、"场景"、"执行状态"、"ark用例执行耗时(ms)"、"昨日ark用例执行耗时(ms)"、"是否劣化"、"v 8(ms)"、"v 8 --jitless(ms)"、"ark/v 8"、"ark/v 8 jitless"、"hyperlink"、"备注"；
       表格最后，会追加汇总信息：用例数总数、执行成功数、失败数，劣化数目等内容。具体内容，请参见附录“daily报告”。
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
- G列 “v 8(ms)”是有基准数据每月1、11、21日会生成v 8执行时间的基准数据，其他时间都不进行v 8执行用例操作，直接获取前边最近的基准数据
- H列 “v 8 --jitless(ms)”是有基准数据每月1、11、21日会生成v 8 --jitless执行时间的基准数据，其他时间都不进行v 8 --jitless执行用例操作，直接获取前边最近的v 8 --jitless基准数据
