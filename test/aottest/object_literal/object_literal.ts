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

// test objectLiteral transition
function test() {
    let obj = {a: 1, b: 2};
    obj.c = 3;
    print(obj.c);
}
test();
print(ArkTools.isAOTCompiled(test))
print(ArkTools.isAOTDeoptimized(test))

// test whether the ObjectLiteralHClassCache is right
function test2() {
    let obj0 = ArkTools.createNapiObject();
    let d = {x: 3, y: 4};
    function foo() {
        obj0.x = 1;
        print(obj0.x);
    }
    foo();
    if (ArkTools.isAOTCompiled(foo)) {
        d = {x: 1, y: 2};
    }
    print(d.x);
    print(d.y);
}
test2();
print(ArkTools.isAOTCompiled(test2))
print(ArkTools.isAOTDeoptimized(test2))
