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

declare function print(arg:any):string;

let d = {x:1, y:2}
d.x = 3;
print(d.x);

function foo(obj) {
   let ret;
   for (let i = 0; i < 10000; i++) {
       ret = obj.x
   }
   return ret
}

let a = {x:1, y: 2}

let b = {x:2, y: 3}

print(foo(a));
print(foo(b));


var originObject = {
    func() {
        return {
            index: 0,
            next() {
                return {
                    value: this.val,
                    done: this.isDone
                };
            },
            isDone: false,
            get val() {
                this.index++;
                if (this.index > 7) {
                    this.isDone = true;
                }
                return 1 << this.index;
            }
        };
    },
    0: "lighter",
    1: "normal",
    2: "regular",
    3: "medium",
    wode: 55,
    "hhh": "wwww",
    func1: function(a) {
        print(a);
    }
};

Object.keys(originObject).forEach(
    function(key) {
        print(key);
        print(originObject[key]);
    }
);

var funn =  originObject.func();

Object.keys(funn).forEach(
    function(key) {
        print(key);
        print(funn[key]);
    }
);