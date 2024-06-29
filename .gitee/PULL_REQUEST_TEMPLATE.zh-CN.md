### 关联的Issue

### 修改原因（目的、解决的问题等，例如：修复xx场景崩溃问题）

### 修改描述（做了什么，变更了什么，例如：xx函数入口增加判空）

### 自测试项（测试结果截图，直接贴到每一个测试项底下）
#### 独立编译进行编译（必须执行 python ark.py arm64.release）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### 独立编译进行编译（必须执行 python ark.py x64.release）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### debug UT 本地执行（必须执行 python ark.py x64.debug ut）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### release UT 本地执行（必须执行 python ark.py x64.release ut）
- [ ] 已通过 
- [ ] 不涉及，无需验证
#### release test262 本地执行（非Test代码必须执行 python ark.py x64.release test262）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### debug test262 本地执行（非Test代码必须执行 python ark.py x64.debug test262）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### release pgo-aot test262 本地执行（compiler/aot/isa/stub/ic/对象布局等修改必须执行）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### debug pgo-aot test262 本地执行（compiler/aot/isa/stub/ic/对象布局等修改必须执行）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### jit-workload 本地执行（compiler/jit/isa/stub/ic/对象布局等修改必须执行）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### regress测试套 本地执行（解释器/builtins/异常处理/对象布局/IC等修改必须执行）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### interpreter-js_perf 性能测试（性能优化/性能敏感场景：IR化/ic/hclass/对象布局/isa/builtins等修改必须执行）
- [ ] 已验证不劣化
- [ ] 不涉及，无需验证
#### aot-js_perf 性能测试（性能优化/性能敏感场景：IR化/ic/hclass/对象布局/isa/builtins等修改必须执行）
- [ ] 已验证不劣化
- [ ] 不涉及，无需验证
#### interpreter-workload性能测试（性能优化/性能敏感场景：IR化/ic/hclass/对象布局/isa/builtins等修改必须执行）
- [ ] 已验证不劣化
- [ ] 不涉及，无需验证
#### aot-workload性能测试（性能优化/性能敏感场景：IR化/compiler/aot/isa/stub/ic/对象布局等修改必须执行）
- [ ] 已验证不劣化
- [ ] 不涉及，无需验证
#### rk3568 L0用例验证，门禁构建镜像启动一遍所有应用（非Test代码必须执行）
- [ ] 已通过
- [ ] 不涉及，无需验证
#### rk3568 wukong压测2小时验证（非Test代码需要，可用原型机替代验证）
- [ ] 无相关crash产生
- [ ] 不涉及，无需验证
#### 非StableArray不能采用ElementKind读取元素（修改解释器相关IR代码需要自检）
- [ ] 已自检
- [ ] 不涉及，无需验证
#### 在IR时添加fastpath时，需要检查是否有分支会改变非字典序模式或者修改数组长度，此时应该走slowpath（修改解释器相关IR代码需要自检）
- [ ] 已自检
- [ ] 不涉及，无需验证
#### IR化后的逻辑需要与c++实现一致，不一致的地方需要有分支用例看护（修改解释器相关IR代码需要自检）
- [ ] 已自检
- [ ] 不涉及，无需验证
#### c++代码中检测或抛出异常的地方，IR需要同样实现（修改解释器相关IR代码需要自检）
- [ ] 已自检
- [ ] 不涉及，无需验证
#### IR中使用默认入参是HeapObject对象的接口时，需要在上下文中保证了参数的正确性（修改解释器相关IR代码需要自检）
- [ ] 已自检
- [ ] 不涉及，无需验证
#### 修改c解释器，需要同时检查汇编解释器与AOT是否需要修改
- [ ] 已自检
- [ ] 不涉及，无需验证
