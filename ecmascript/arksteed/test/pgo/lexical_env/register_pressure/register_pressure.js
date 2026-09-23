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
//! METHOD pressure
//! HAS LoadTaggedElement
//! HAS StoreEnvSlot
//! HAS SetValueWithBarrier
//! HAS_NOT CallCommonStub GetPropertyByValue

function createPressure()
{
    let capturedInt = 0;
    let capturedObject = { marker: 0 };

    function pressure(array, index, flag, object)
    {
        let value0 = array[index];
        let value1 = array[index];
        let result0 = value0 + value1;
        let result1 = result0 + 1;
        let result2 = result1 + 2;
        let result3 = result2 + 3;
        let result4 = result3 + 4;
        let result5 = result4 + 5;
        let result6 = result5 + 6;
        let result7 = result6 + 7;
        let result8 = result7 + 8;
        let result9 = result8 + 9;
        let result10 = result9 + 10;
        let result11 = result10 + 11;

        capturedInt = flag ? 1 : 2;
        capturedObject = object;
        return [result0, result1, result2, result3, result4,
                result5, result6, result7, result8, result9,
                result10, result11];
    }

    function read()
    {
        return [capturedInt, capturedObject];
    }

    return { pressure, read };
}

const fixture = createPressure();
const values = [10, 20, 30, 40];
const object = { marker: 9 };
for (let i = 0; i < 1000; i++) {
    fixture.pressure(values, i & 3, (i & 1) === 0, object);
}

ArkTools.arkSteedCompileSync(fixture.pressure);
ArkTools.waitJitCompileFinish(fixture.pressure);

const result = fixture.pressure(values, 1, true, object);
const captured = fixture.read();
let ok = ArkTools.isAOTCompiled(fixture.pressure);
ok = ok && result.length === 12;
ok = ok && result[0] === 40;
ok = ok && result[11] === 106;
ok = ok && captured[0] === 1;
ok = ok && captured[1] === object;

print(ok ? "PASS" : "FAIL");
