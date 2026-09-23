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

function storeElement(receiver, value) {
    receiver[1] = value;
}

let intArray = [1, 2, 3];
let numberArray = [1.5, 2.5, 3.5];
let taggedArray = [1, "two", { marker: 3 }];
for (let i = 0; i < 30; i++) {
    switch (i % 3) {
        case 0:
            storeElement(intArray, i);
            break;
        case 1:
            storeElement(numberArray, i + 0.5);
            break;
        default:
            storeElement(taggedArray, "warm");
            break;
    }
}

print(ArkTools.getICState(storeElement, 0, 3));
ArkTools.arkSteedCompileSync(storeElement);

let objectValue = { marker: 41 };
storeElement(intArray, objectValue);
print(intArray[1].marker);
print(ArkTools.getElementsKind(intArray));

storeElement(numberArray, "forty-two");
print(numberArray[1]);
print(ArkTools.getElementsKind(numberArray));

storeElement(taggedArray, 43);
print(taggedArray[1]);
print(ArkTools.getElementsKind(taggedArray));
