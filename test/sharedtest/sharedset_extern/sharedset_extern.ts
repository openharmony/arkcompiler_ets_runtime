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

// @ts-nocheck
declare function print(str: any): string;

print("===Class inheritance test begin ===");
class SubSendableSet<T> extends SendableSet {
  desc: string = "I'am SubSendableSet";
  constructor(entries?: T[] | null) {
    'use sendable';
    super(entries);
  }
}

let subSharedset = new SubSendableSet<number>();
subSharedset.add(1);
print(subSharedset.has(1));
print(subSharedset.size);

subSharedset = new SubSendableSet<number>([1, 2, 3]);
print(subSharedset.has(1));
print(subSharedset.has(2));
print(subSharedset.has(3));
print(subSharedset.size);

try {
  let obj = {};
  subSharedset = new SubSendableSet<Object>([obj]);
  print(subSharedset.size);
} catch (e) {
  print('SubSendableSet add[unshared]: ' + e + ', errCode: ' + e.code);
}

subSharedset = new SubSendableSet<string>(['one', 'two', 'three']);
for (const [key, _] of subSharedset.entries()) {
  print('SubSendableSet key[for-of]: ' + key);
}

try {
  subSharedset = new SubSendableSet<number>([1, 2, 3, 4]);
  print(subSharedset.size);
  subSharedset.forEach((key: number, _: number, set: SubSendableSet) => {
    if (key % 2 == 0) {
      set.delete(key);
    }
  });
} catch (e) {
  print('SubSendableSet Delete Scenario[forEach]: ' + e + ', errCode: ' + e.code);
}

class SubSubSendableSet<T> extends SubSendableSet {
  constructor(entries?: T[] | null) {
    'use sendable';
    super(entries);
  }
}

let subSubSendableSet = new SubSubSendableSet<number>([1, 2, 3]);
print(subSubSendableSet.has(1));
print(subSubSendableSet.has(2));
print(subSubSendableSet.has(3));
print(subSubSendableSet.size);

try {
  subSubSendableSet['extension'] = 'value';
} catch(e) {
  print("add extension(.): " + e);
}
try {
  subSubSendableSet.extension = 'value';
} catch(e) {
  print("add extension([]): " + e);
}

try {
  let obj = {};
  subSubSendableSet = new SubSubSendableSet<Object>([obj]);
  print(subSubSendableSet.size);
} catch (e) {
  print('SubSubSendableSet add[unshared]: ' + e + ', errCode: ' + e.code);
}

try {
  subSubSendableSet = new SubSubSendableSet<number>([1, 2, 3, 4]);
  subSubSendableSet.forEach((key: number, _: number, set: SubSubSendableSet) => {
    if (key % 2 == 0) {
      set.delete(key);
    }
  });
} catch (e) {
  print('SubSubSendableSet Delete Scenario[forEach]: ' + e + ', errCode: ' + e.code);
}

print("===Class inheritance test end ===");
