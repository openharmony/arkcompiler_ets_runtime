/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! COMPILE_MODE script
//! PARAMS --compiler-jit-hotness-threshold=10000
//! METHOD writeThenRead
//! HAS StoreTaggedFieldWithBarrier
//! COUNT_LE DeoptIfTaggedCondition 2
//! COUNT_LE LoadTaggedField 2
//! HAS_NOT CallCommonStub StGlobalVar

var globalWriteReadValue = 0;

function writeThenRead(value)
{
    globalWriteReadValue = value;
    return globalWriteReadValue;
}

for (let i = 0; i < 1000; i++) {
    writeThenRead(i);
}

ArkTools.arkSteedCompileSync(writeThenRead);
ArkTools.waitJitCompileFinish(writeThenRead);

let ok = ArkTools.isAOTCompiled(writeThenRead);
ok = ok && writeThenRead(10) === 10;
ok = ok && globalThis.globalWriteReadValue === 10;
ok = ok && writeThenRead(20) === 20;
ok = ok && globalThis.globalWriteReadValue === 20;

print(ok ? "PASS" : "FAIL");
