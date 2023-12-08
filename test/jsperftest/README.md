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
1) 调用入口：bash run_js_perf_test.sh $js_report_save_path
   * js_report_save_path：表示报告输出的目录路径，该路径同时也是历史报告路径
   * 请在该文件中修改第5行的OPENHARMONY_OUT_PATH变量，将鸿蒙系统out目录的路径补充上，适配你的测试环境。注意是out目录的父目录路径。
   
3) 运行生成看护报告说明
    ```bash
    1. jsperftest生成的daily报告，excel格式
    	命名格式：js_perf_test_result_年月日.xlsx，比如js_perf_test_result_20231206.xlsx。
    2. 报告表格表头输出"用例名称"、"执行状态"、"用例执行耗时(ns)"、"昨日用例执行耗时(ns)"、"是否劣化"、"执行日志"、"备注"；
       表格最后，会追加汇总信息：用例数总数、执行成功数、失败数，劣化数目等内容。具体内容，请参见附录“daily报告”。
    3. 有执行失败时，当前用例的“执行状态”单元格内容会标记。
    4. “是否劣化”列，取值：true，false。当用例前一天性能数据不存在、js用例执行异常、执行失败，都归于没有劣化。
    ```

## daily报告

请参见附录"Daily报告"。

## workload代码仓

  [Ark-workload](https://gitee.com/dov1s/arkjs-perf-test/tree/builtins_test1110/)

## 附录

### valueOf.js用例执行日志：

```bash
[gc] Ark Auto adapter memory pool capacity:6845104128
[compiler] loaded stub file successfully
NaN
NaN
time: 0
NaN
NaN
time: 2
NaN
NaN
time: 0
NaN
NaN
time: 0
[ecmascript] Runtime State duration:2740100(ns)
[ecmascript] panda runtime stat:
[ecmascript]     InterPreter && GC && C++ Builtin Function    Time(ns)       Count MaxTime(ns) AvgTime(ns)
[ecmascript] =====================================================================================================================
[ecmascript] ---------------------------------------------------------------------------------------------------------------------
[ecmascript]                    Interpreter Total Time(ns)           0
[ecmascript]                    BuiltinsApi Total Time(ns)           0
[ecmascript]              AbstractOperation Total Time(ns)           0
[ecmascript]                         Memory Total Time(ns)           0
[ecmascript]                        Runtime Total Time(ns)           0
[ecmascript] ---------------------------------------------------------------------------------------------------------------------
[ecmascript]                                Total Time(ns)           0
[ecmascript] remove js pandafile by vm destruct, file:/home/lhb/test/valueOf.abc
```

### Daily报告

| 用例名称                        | 执行状态 | 用例执行耗时（ns) | 昨日用例执行耗时(ns) | 是否劣化 | 执行日志                                        | 备注                             |
| ------------------------------- | -------- | ----------------- | -------------------- | -------- | ----------------------------------------------- | -------------------------------- |
| Reflect/deleteProperty.js       | pass     | 285600            | 285612               | False    |                                                 |                                  |
| Reflect/deleteProperty.js       | pass     | 118700            | 117900               | False    |                                                 |                                  |
| Reflect/construct.js            | pass     | 129000            | 140182               | True     | [gc] Ark Auto adapter memory pool capacity..... | last day excute time(ns): 140182 |
| Reflect/getPrototypeOf.js       | pass     | 120300            | 120100               | False    |                                                 |                                  |
| Array/to-spliced.js             | fail     |                   | 539500               | False    | [gc] Ark Auto adapter memory pool capacity..... |                                  |
|                                 |          |                   |                      |          |                                                 |                                  |
|                                 |          |                   |                      |          |                                                 |                                  |
| Degraded percentage upper limit | 0.05     |                   |                      |          |                                                 |                                  |
| Totle js case count             | 5        |                   |                      |          |                                                 |                                  |
| Pass count                      | 4        |                   |                      |          |                                                 |                                  |
| Fail count                      | 1        |                   |                      |          |                                                 |                                  |
| Degraded count                  | 1        |                   |                      |          |                                                 |                                  |
| Total excute time is(s)         | 00:00:01 |                   |                      |          |                                                 |                                  |

说明：

- “昨日用例执行耗时（ns)”默认是隐藏的。
- 执行日志的内容，请参见示例：附录“valueOf.js用例执行日志”。
- 公里执行失败、劣化时，都会有日志信息
