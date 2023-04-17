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

// @ts-nocheck
declare function print(str:any):string;
declare var ArkTools:any;
function sum(a:number, b:number):number {
    return a + b;
}

const handler = {
    apply: function(target:any, thisArg:any, argumentsList:any[]) {
        print(`Calculate sum: ${argumentsList}`);
        // expected output: "Calculate sum: 1,2"
        return target(argumentsList[0], argumentsList[1]) * 10;
   }
};

print(sum(1, 2));
// call proxy-forward-AOT
const proxy0 = new Proxy(sum, handler);
print(proxy0(1, 2));

// call proxy-forward-asm
ArkTools.removeAOTFlag(handler.apply);
const proxy1 = new Proxy(sum, handler);
print(proxy1(1, 2));

// call proxy-no-forward-AOT
const proxy2 = new Proxy(sum, {});
print(proxy2(1, 2));

// call proxy-no-forward-AOT
const proxy3 = new Proxy(sum, {});
print(proxy3(1, 2, 3));

// call proxy-no-forward-bound-AOT
const proxy4 = new Proxy(sum.bind({}, 3), {});
print(proxy4(1, 2));

// call proxy-no-forward-native-AOT
const proxy5 = new Proxy(print, {});
print(proxy5(1, 2));

// call proxy-no-forward-asm
ArkTools.removeAOTFlag(sum);
const proxy6 = new Proxy(sum, {});
print(proxy6(1, 2));