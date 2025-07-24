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

class Foo { }
function Bar() { }
var doBadThings = function () { };

Bar.prototype = new Proxy({}, {
    getPrototypeOf() {
        doBadThings();
        return Foo.prototype;
    }
});

function foo(o, p) {
    var result = o.f;
    var _ = p instanceof Foo;
    return result + o.f;
}

var globalO = { f: 1 };
doBadThings = function () { globalO.f = 2; };
var result = foo(globalO, new Bar());
if (result != 3) {
    throw "Error: bad result: " + result;
}
print(ArkTools.isAOTCompiled(foo));
