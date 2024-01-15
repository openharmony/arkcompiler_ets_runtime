# 运行ffi workload性能用例，呈现ark流水线daily报告

## 介绍

自动化运行workload性能测试用例并且生成daily报告，呈现性能用例的每日执行情况，并提供与前一日耗时的劣化对比情况

## 目录

```bash
/arkcompiler
│_ test                               # 模块测试用例
    |_ ffiworkloadtest                # ffi workload 测试集成ark流水线脚本目录
		 |-README.md                  # 说明书
		 |-run_ffi_workload_test.sh   # 测试执行shell脚本
		 |_get_ffi_workload_report.py # 测试执行调用的python脚本
		 
```

## 环境依赖

python3、unzip、openpyxl、hdc >=1.3.0

## 运行测试

1. 调用入口：run_ffi_workload_test.sh
   指令格式：`bash run_ffi_workload_test.sh $run_mode $openharmony_root_path   $daily_report_output_path` $execution_machine_ip
   调用示例：`bash run_ffi_workload_test.sh 1 /root/openharmony   /root/daily_report_output_path/`172.24.32.1:8710
   运行参数说明如下：
   
   | 参数名称   | 参数含义 | 备注 |
   | --------| -------- | -------------------- |
   | run_mode | [必选] sh脚本执行是在蓝区（开放网络环境）还是黄区（限制网络环境）。1--蓝区；2-黄区。目前支持rk3568编译 |  |
   | openharmony_root_path | [必选] harmony镜像根目录。out的父目录。harmony必须全编译通过，生成out目录。 |  |
   | daily_report_output_path | [必选] daily报告存储路径 |  |
   | execution_machine_ip | [必选] 开发板或harmony手机通过USB直接连接的PC的linux环境的IP&端口。格式示例：172.24.32.1:8710， 8710是默认hdc设备端口。默认连接ark流水线的是实体机是通过USB直接连接的。IP和端口在PC环境CMD窗口通过执行ipconfig/all查询得到 | |


2. 生成流水线报告
    ```bash
    1). ffiworkloadtest生成的daily报告，excel格式
        命名格式：ffi_worklaod_daily_report_年月日.xlsx，比如ffi_worklaod_daily_report_20231206.xlsx。
    2). 报告表格表头输出"用例名称"、"执行状态"、"用例执行耗时-1(s)"、"用例执行耗时-2(s)"、"用例执行耗时-3(s)"、"用例执行耗时-4(s)"、"用例执行耗时-5(s)"、"当日用例平均执行耗时(s)"、"昨日用例平均执行耗时(s)"、"是否劣化"、"备注"；
       表格末尾，有汇总信息：用例数总数、用例执行成功数量、失败数量、禁用数量、忽略的数量、报错的数量、劣化数量、当日平均执行耗时、劣化上限值、测试报告生成日期等内容。
    3). 有执行失败时，当前用例的“执行状态”单元格内容会标记。当日未执行则标记为"NA"；执行成功标记"pass";执行失败标记"fail"
    4). “是否劣化”列，取值：true，false, NA。当用例前一天性能数据不存在、用例执行异常、执行失败，都归于"NA"状态。
    ```

## workload 用例位置

  [ffi_workload.cpp位置](https://gitee.com/openharmony/arkcompiler_ets_runtime/blob/%6d%61%73%74%65%72/ecmascript/napi/test/ffi_workload.cpp)