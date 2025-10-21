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

function foo() {
    return this.f;
}

String.prototype.f = 43;
String.prototype.g = foo;
Number.prototype.f = 78;
Number.prototype.g = foo;

function Test() {
    var o = {f:foo};
    var result = o.f();
    o = 42;
    result = o.g();
    print(result);
}

Test()
ArkTools.jitCompileAsync(foo);
print(ArkTools.waitJitCompileFinish(foo));
Test()