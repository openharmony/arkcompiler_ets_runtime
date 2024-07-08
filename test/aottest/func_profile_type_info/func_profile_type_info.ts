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

declare function print(str:string):string;

function defineFunc(): Object {
  let cnt: number = 10;
  let result: Object = {};
  let cb = () => {
    // the profileTypeInfo of lambda is not bind on the function defineFunc.
    // This leads to frequent "ic miss" when execute "result.year = 1"
    result.year = 1;
  };
  for (let i: number = 0; i < cnt; ++i) {
    cb();
  }
  return cb;
}

function defineClass(): Object {
    class ReplaceClass{
        constructor(value) {
            // the profileTypeInfo of constructor is not bind on the function defineClass.
            // This leads to frequent "ic miss" when execute "this.value=value"
            this.value=value;
        }
    }
    new ReplaceClass();
    return ReplaceClass;
}

function defineMethod(): Object {
    class MyClass {
      static [Symbol.hasInstance](instance): boolean {
           // the profileTypeInfo of constructor is not bind on the function defineMethod.
           // This leads to frequent "ic miss" when execute "Array.isArray"
        return Array.isArray(instance);
      }
    }

    [] instanceof MyClass;
    return MyClass[Symbol.hasInstance];
}

function run(defined: () => Object): Object[] {
  let cnt: number = 100;
  let cbArr: Object[] = [];
  for (let i: number = 0; i < cnt; ++i) {
    cbArr.push(defined());
  }
  return cbArr;
}

function assert_true(condition: boolean):void
{
    if (!condition) {
        print("assert fail");
    }
}

function CheckProfileTypeInfoNotChange(cbArr: Object[]): void {
    let len: number = cbArr.length;
    assert_true(len > 1);
    let base = cbArr[0];
    for (let i: number = 0; i < len; ++i) {
        let checkTarget = cbArr[i];
        assert_true(ArkTools.isProfileTypeInfoValid(checkTarget));
        assert_true(ArkTools.isSameProfileTypeInfo(base, checkTarget));
    }
}

CheckProfileTypeInfoNotChange(run(defineFunc));
assert_true(ArkTools.isAOTCompiled(defineFunc));
CheckProfileTypeInfoNotChange(run(defineMethod));
assert_true(ArkTools.isAOTCompiled(defineMethod));
CheckProfileTypeInfoNotChange(run(defineClass));
assert_true(ArkTools.isAOTCompiled(defineClass));

print("success");
