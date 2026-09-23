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

//! PARAMS --compiler-jit-hotness-threshold=10000
//! METHOD set
//! HAS StoreEnvSlot
//! HAS SetValueWithBarrier

function createHolder()
{
    let captured = undefined;

    function set(value)
    {
        captured = value;
        return captured;
    }

    return set;
}

const set = createHolder();
const object = { payload: 1 };
for (let i = 0; i < 1000; i++) {
    set(object);
}

ArkTools.arkSteedCompileSync(set);
ArkTools.waitJitCompileFinish(set);

let ok = ArkTools.isAOTCompiled(set);
for (let i = 0; i < 2048; i++) {
    ok = ok && set(object) === object;
    if ((i & 127) === 0) {
        ArkTools.gc();
    }
}
ok = ok && object.payload === 1;

print(ok ? "PASS" : "FAIL");
