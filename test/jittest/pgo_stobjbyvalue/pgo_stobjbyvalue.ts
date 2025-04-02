/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

// @ts-nocheck
declare function print(arg: any): string;

var leak;
function test3() {
    var a = [1];
    a[1] = 2;
    return a[0] + a[1];
}

print(test3());
Object.defineProperty(Array.prototype, "1", { get: function () { print("get"); return 4; }, set: function () { leak = this; print("set"); } });
print(test3());
ArkTools.jitCompileAsync(test3);
print(ArkTools.waitJitCompileFinish(test3));
print(test3());
print(leak[0]);
print(leak[1]);

class Index {
    constructor() {
        this.textArray = new Array(27);
    }
    setElement() {
        for (let i = 0; i < 40; i++) {
            this.textArray[i] = 1;
        }
    }
}

function main() {
    let indexObj = new Index()
    indexObj.setElement()
    print("done")
}
main()
ArkTools.jitCompileAsync(main);
print(ArkTools.waitJitCompileFinish(main));
main()
