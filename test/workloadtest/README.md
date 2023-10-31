# 运行方舟编译器工作负载

### 介绍

自动化运行性能测试用例并且生成daily报告，用于测试 ark 编译器的性能。

## 目录

```
/arkcompiler/ets_runtime
│─ test                   # 模块测试用例
|   ├─ workloadtest       # Ark-workload测试用例仓、py脚本
|        ├─ data          # 测试用例仓
```

### 运行测试
1) 调用入口：python3 ark.py x64.release workload report
   * workload：表示运行workload测试用例
   * report：生成看护报告
2)  python3 arkcompiler/ets_runtime/test/workloadtest/work_load.py --report True --code-path='/workspace/openharmony/code' --run-count=1 --boundary-value=-20：
    * report： 生成看护报告
    * code-path： your code path
    * tools_type：tools type,dev or rk3568 or hispark_taurus, 默认是dev
    * run-count：表用例运行次数，默认是10次。
    * boundary-value：劣项边界值，默认是-10。用于标记看护报告的"difference"列。
3) 运行生成看护报告说明
    ```
    1. workload生成的Excel，命名格式：pgo_data_年月日时分秒.xlsx，daily报告是依赖于上述Excel而生成的。（时间最近的2个excel差值占比输出到daily）
    2. 报告命名格式：pgo_daily.xlsx，每个用例的平均差异百分比，其计算规则相同：
    （当天caseXXX的average值 - 前一天caseXXX的average值）/ 当天caseXXX的average值
    3. 占比值小于劣项边界值时，当前单元格会标记为红色。
    ```

### workload仓
  [Ark-workload](https://gitee.com/xliu-huanwei/ark-workload)

