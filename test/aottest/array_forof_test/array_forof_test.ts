/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}

// Test basic function
function func1() {
    let arr = [1, 2]
    for (let ele of arr) {
        print(ele);
    }
}

function func2() {
    let typedArr = new Uint32Array([1,2]);
    for (let ele of typedArr) {
        print(ele);
    }
}

function func3() {
    var typedArrForTestIter2 = new Uint32Array([1,2]);
    var iterForTypedArrEntry = typedArrForTestIter2.entries();
    for (let typedArrEle of iterForTypedArrEntry) {
        print(typedArrEle);
    }
}

function func4() {
    let arrayLike = {
        get length() {
            return 2;
        },
        0: 'a',
        1: 'b'
    };

    for (let value of Array.prototype[Symbol.iterator].call(arrayLike)) {
        print(value);
    }

    // Test Entries
    let result;
    const entriesIterator = Array.prototype.entries.call(arrayLike);
    while (!(result = entriesIterator.next()).done) {
        print(result.value, result.done);
    }
    print(result.value, result.done);
}

// Exceptions
function func5() {
  const arrayLike = {
      get length() {
          throw new Error("Access to length is denied");
          
      },
      0: 'a',
      1: 'b'
  };
  
  const iterator = Array.prototype[Symbol.iterator].call(arrayLike);
  
  try {
      iterator.next()
  } catch {
      print("Exception")
  }
}

func1()
func2()
func3()
func4()
func5()
print(ArkTools.isAOTCompiled(func1))
print(ArkTools.isAOTCompiled(func2))
print(ArkTools.isAOTCompiled(func3))
print(ArkTools.isAOTCompiled(func4))
print(ArkTools.isAOTCompiled(func5))
