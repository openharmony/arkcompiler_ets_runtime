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
//! HAS_NOT SetValueWithBarrier

function outer()
{
    let captured = 0;

    function middle()
    {
        function set()
        {
            captured = 1;
            return captured;
        }

        return set;
    }

    return middle();
}

const set = outer();
for (let i = 0; i < 1000; i++) {
    set();
}

ArkTools.arkSteedCompileSync(set);
ArkTools.waitJitCompileFinish(set);

let ok = ArkTools.isAOTCompiled(set);
ok = ok && set() === 1;

print(ok ? "PASS" : "FAIL");
