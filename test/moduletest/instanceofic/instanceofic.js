/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

/*
 * @tc.name:instanceofic
 * @tc.desc:test instanceofic
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

class F {
    static [Symbol.hasInstance](instance) {
        return Array.isArray(instance);
    }
};
let res;

for (let i = 300; i > 0; --i) {
    res = [] instanceof F;
    if (i == 150) {
        if (res) {
            print("instanceoficsuccess");
        } else {
            print("instanceoficfailse");
        }
        F[Symbol.hasInstance] = function() { return false };
    }
    if (i == 140) {
        if (!res) {
            print("instanceoficsuccess");
        } else {
            print("instanceoficfailse");
        }
    }
}

let e = new Error("instanceof ic failed")
for (let index = 0; index <= 30; index++) {
    if ((e instanceof URIError) === true){
        continue
    }
}

print("test success")

for (let index = 0; index <= 30; index++) {
    function foo() {
        try {
            const obj = {};
            obj instanceof {};
        } catch (e) {

        }
    }
    foo();
}
print("test success")

// Test instanceof when hasInstance is modified uncallable
var instanceObj = {
    [Symbol.hasInstance]: 1.1
}
try {
    print({} instanceof instanceObj);
} catch (e) {
    print("CallbackFn is not callable");
}

// Test instanceof if constructor's prototype has been modified
function function_prototype_changed() {

}
try {
    function_prototype_changed.prototype = 5;
    Array instanceof function_prototype_changed;
} catch (e) {
    print(e.name)
}

var A = {};
function Func() { }
Func.prototype = A;
print(A instanceof Func);

var proto_desc = Object.getOwnPropertyDescriptor(RegExp, "prototype");
var proto = proto_desc.value;
print(proto instanceof RegExp);
