/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 * @tc.name:fromCharCode
 * @tc.desc:test String.fromCharCode and charat
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

var obj = {};
try {
Object.defineProperties(obj, {
    foo: {value:1},
    bar: {value:2, get:function() {return 3;}}
});
} catch (e) {
    print(e)
}
print(obj.foo);
print(obj.bar);

var obj1 = {};
try {
Object.defineProperties(obj1, {
    foo: {value:1},
    bar: {value:2, enumerable:false},
    hhh: {value:3, enumerable:true, writable:false},
});
} catch (e) {
    print(e)
}
print(obj1.foo);
print(obj1.bar);
print(obj1.hhh);

function f0() {
    return 1;
}

Object.defineProperty(f0, "length", { enumerable: true, get: f0 });
print(f0.length);
