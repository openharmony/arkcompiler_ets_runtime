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
//! METHOD update
//! HAS StoreEnvSlot
//! HAS SetValueWithBarrier

function createMixedStore()
{
    let captured = 0;

    function update(flag, value)
    {
        captured = flag ? 1 : value;
        return captured;
    }

    return update;
}

const update = createMixedStore();
const object = { marker: 2 };
for (let i = 0; i < 1000; i++) {
    update((i & 1) === 0, object);
}

ArkTools.arkSteedCompileSync(update);
ArkTools.waitJitCompileFinish(update);

let ok = ArkTools.isAOTCompiled(update);
ok = ok && update(true, object) === 1;
ok = ok && update(false, object) === object;

print(ok ? "PASS" : "FAIL");
