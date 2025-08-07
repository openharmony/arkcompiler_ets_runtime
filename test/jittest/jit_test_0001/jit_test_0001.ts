/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

function Test(cond) {
    var value = undefined
    if (cond) {
        value = 1.2 + 1.3
    } else {
        value = {
            id: 268436140
	}
    }
    return value
}

for (var i = 0; i < 10; i++) {
    var cond = ((i % 2 == 1) ? true : false)
    Test(cond)
}

ArkTools.jitCompileAsync(Test);
var ret = ArkTools.waitJitCompileFinish(Test);
print(ret);

function Test2() {
    let v2 = new Uint8ClampedArray(42);
    const v5 = new Int32Array(3);
    ({"buffer": v2} = v5);

    function func() {
        return v5;
    }
    let v21 = 10;
    for (;v21--;) {
        v2.valueOf = func;
    }
}

Test2();
ArkTools.jitCompileAsync(Test2);
print(ArkTools.waitJitCompileFinish(Test2));
Test2();
