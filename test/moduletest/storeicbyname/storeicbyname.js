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
 * @tc.name:storeicbyname
 * @tc.desc:test storeicbyname
 * @tc.type: FUNC
 * @tc.require: issueI7UTOA
 */

const arr = [];
for (let i = 0; i < 200; i++) {

}
for (let i = 0; i < 100; i++) {
    arr.length = 1025;
}

for (let i = 0; i < 100; i++) {
    const v77 = {};
    v77.__proto__ = null;
}

let bad_proto = {
    get __proto__(){

    }
}
let obj = {};
obj.__proto__ = bad_proto;

let obj1 = {};
Object.defineProperty(obj1, 'foo', {
    set: function(value) {
        Object.defineProperty(this, 'bar', {
            get: function() {
                return this._bar;
            },
            set: function(value) {
                this._bar = value;
            },
        });
    }
});
 
obj.foo = 'some value';
for (let i = 0; i < 20; i++) {
    obj.bar = 'new value';
}

{
    let o = {};
    o["a"] = String.prototype;
    for (let i = 0; i < 2; i++) {
        let o1;
        if (o1) {
            o["a"]= o1;
        } else {
            o1 = o["a"];
        }
        for (let j = 0; j < 50; j++) {
            let o2 = new Object(-63);
            if (o2) {
                o["a"] = o2;
            } else {
                o2 = o["a"];
            }
            let o3 = o1.valueOf();
            if (o3) {
                o["a"] = o3;
            } else {
                o3 = o["a"];
            }
            // o3 is Number when i = 0 and o3 is Int when i = 1
            try {
                o3.x = "telu";
            } catch(e) {}
        }
    }
}

test_end();