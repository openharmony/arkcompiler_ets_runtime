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

class A {}

let var0 = 0;

Object.defineProperty(globalThis, 'var1', {
  value: 1,
  configurable: true
});


function TestLoadA() {
    A.a = 1;
    var0 = 1;
    return A.a + var0;
}

function TestBuiltinMath() {
    return Math.E;
}

function TestAccessor() {
    let a = var1;
    return a;
}

function TestAccessor1() {
    Object.defineProperty(globalThis, 'var1', {
        configurable: true,
	get: function () {print("TestAccessor1");return 2;}
    });
}

for (var i = 0; i < 10; i++) {
    TestLoadA();
    TestAccessor();
    TestBuiltinMath();
}

ArkTools.jitCompileAsync(TestLoadA);
var ret = ArkTools.waitJitCompileFinish(TestLoadA);
print(ret)
print(TestLoadA());

ArkTools.jitCompileAsync(TestAccessor);
ret = ArkTools.waitJitCompileFinish(TestAccessor);
print(ret)
print(TestAccessor())

// invaild property box
TestAccessor1();
// deopt
print(TestAccessor());

ArkTools.jitCompileAsync(TestBuiltinMath);
ret = ArkTools.waitJitCompileFinish(TestBuiltinMath);
print(ret)
print(TestBuiltinMath());

