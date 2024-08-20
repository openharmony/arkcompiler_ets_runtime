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

export class Test {
  constructor(a) {
    this.a = a;
  }

  fun1() {
    print('hello world');
  }

  fun2() {
    print('hello world1');
  }
}

export let module = 0;

let moduleName = 'default';
try {
  moduleName = __getCurrentModuleName__();
} catch (error) {
  moduleName = 'error';
}

export {moduleName};