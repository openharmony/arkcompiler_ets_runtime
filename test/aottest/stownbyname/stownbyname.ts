/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str:any):string;
var foo = {
    bar: [ "a", "b" ]
  };
print(foo.bar[0]);
print(foo.bar[1]);

interface objInterface {
    [key: string]: any
}
let obj:objInterface = {};
obj.a = 100;
obj.b = "helloworld";
print(obj.a)
print(obj.b);

function ArkFunc(depth: number, tag: string): {
    array: number[],
    string: string
} | {
    left: object,
    right: object
} {
    if (depth == 0) {
        return {
            array: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
            string: 'Ark ' + tag + ' Departure!'
        };
    } else {
        return {
            left: ArkFunc(depth - 1, tag),
            right: ArkFunc(depth - 1, tag)
        };
    }
}

var arkDepth: number = 5;
var arkTag: number = 18;
var arkResult = ArkFunc(arkDepth, String(arkTag));