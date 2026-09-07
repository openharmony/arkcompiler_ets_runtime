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
//! METHOD write
//! HAS StoreEnvSlot
//! HAS SetValueWithBarrier
//! COUNT_LE LoadTaggedField 5          # Store-to-load forwarding avoids a second slot read after the store.

function createWriter()
{
    let captured = undefined;

    function write(value)
    {
        captured = value;
        return captured;
    }

    return write;
}

const write = createWriter();
const object = { marker: 1 };
for (let i = 0; i < 1000; i++) {
    write(object);
}

ArkTools.arkSteedCompileSync(write);
ArkTools.waitJitCompileFinish(write);

let ok = ArkTools.isAOTCompiled(write);
ok = ok && write(object) === object;

print(ok ? "PASS" : "FAIL");
