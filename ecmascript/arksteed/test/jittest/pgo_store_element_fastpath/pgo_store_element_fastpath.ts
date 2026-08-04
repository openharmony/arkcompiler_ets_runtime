/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// @ts-nocheck
declare function print(value: any): void;

function storeFastPaths(tagged, taggedPrimitive, objects, ints, doubles, strings, index,
                        taggedValue, objectValue, intValue, doubleValue, stringValue) {
    tagged[index] = taggedValue;
    taggedPrimitive[index] = 123;
    objects[index] = objectValue;
    ints[index] = intValue;
    doubles[index] = doubleValue;
    strings[index] = stringValue;

    let fresh = [1, 2, 3];
    fresh[index] = intValue;
    return fresh[index];
}

let tagged = [1, "two", { marker: 3 }];
let taggedPrimitive = [1, "two", { marker: 3 }];
let objects = [{ marker: 1 }, { marker: 2 }, { marker: 3 }];
let heapValue = { marker: 41 };
let objectValue = { marker: 45 };
let ints = [1, 2, 3];
let doubles = [1.5, 2.5, 3.5];
let strings = ["one", "two", "three"];
for (let i = 0; i < 20; i++) {
    storeFastPaths(tagged, taggedPrimitive, objects, ints, doubles, strings, 1,
                   heapValue, objectValue, -7, 6.25, "warm");
}
ArkTools.arkSteedCompileSync(storeFastPaths);

print(storeFastPaths(tagged, taggedPrimitive, objects, ints, doubles, strings, 1,
                     heapValue, objectValue, -19, -7.75, "forty-three"));
print(tagged[1].marker);
print(taggedPrimitive[1]);
print(objects[1].marker);
print(ints[1]);
print(doubles[1]);
print(strings[1]);

storeFastPaths(tagged, taggedPrimitive, objects, ints, doubles, strings, 1,
               42, objectValue, -19, -7.75, 44);
print(tagged[1]);
print(strings[1]);
