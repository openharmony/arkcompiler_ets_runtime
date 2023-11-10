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
function fun1(){
    let flag0 = false, flag1 = false;
    let _;

    let iterCount_1 = 0;
    async function * fn_1() {
        for await ([ _ = flag0 = true, _ = flag1 = true ] of [[14]]) {
        print(flag0);
        print(flag1);
        iterCount_1 += 1;
        }
    }
    let promise_1 = fn_1().next();
    promise_1
        .then(() => print(iterCount_1));
}
fun1();

function fun2(){
    let y_1 = null;

    let iterCount_2 = 0;
    async function * fn_2() {
        for await ([...[y_1]] of [[ , ]]) {
        print(y_1);
        iterCount_2 += 1;
        }
    }
    let promise_2 = fn_2().next();

    promise_2
        .then(() => print(iterCount_2));
}

fun2();

function fun3(){
    let z, setValue_1;
    z = {
      get y() {
        print('The property should not be accessed.');
      },
      set y(val_1) {
        setValue_1 = val_1;
      }
    };

    let iterCount_3 = 0;
    async function * fn() {
      for await ([z.y] of [[23]]) {
        print(setValue_1);
        iterCount_3 += 1;
      }
    }

    let promise_3 = fn().next();

    promise_3
      .then(() => print(iterCount_3));
}
fun3();
