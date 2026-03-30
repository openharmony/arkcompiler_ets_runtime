/*
* Copyright (c) 2026 Huawei Device Co., Ltd.
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

const observedSymbol = Symbol('____observed_object_name');
globalThis.__OBSERVED_OBJECT_NAME = observedSymbol;

const target1 = { value: 1 };
const handler1 = { [observedSymbol]: 'ClassA' };
globalThis.proxyA = new Proxy(target1, handler1);

const target2 = { value: 2 };
const handler2 = { [observedSymbol]: 'ClassB' };
globalThis.proxyB = new Proxy(target2, handler2);

const target3 = { value: 3 };
const handler3 = {};
globalThis.proxyNoName = new Proxy(target3, handler3);
