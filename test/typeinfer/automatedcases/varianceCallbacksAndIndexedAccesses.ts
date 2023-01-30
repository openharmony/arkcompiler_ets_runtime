/*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Copyright (c) 2023 Huawei Device Co., Ltd.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/varianceCallbacksAndIndexedAccesses.ts ===
declare function AssertType(value:any, type:string):void;
type Source = {
    <K extends keyof WindowEventMap>(type: K, listener: (this: Window, ev: WindowEventMap[K]) => any, options?: boolean | AddEventListenerOptions): void;
  (type: string, listener: EventListenerOrEventListenerObject, options?: boolean | AddEventListenerOptions): void;
}

interface Action1<T> {
    (arg: T): void;
}
interface MessageEventLike<T> {
    source: WindowLike<T>;
    origin: string;
    data: T;
}
interface PostMessageObject<T> {
    postMessage(message: T, host: string): void;
}
interface WindowLike<T> extends PostMessageObject<T> {
    addEventListener(type: "message", handler: Action1<MessageEventLike<T>>): void;
    addEventListener(type: string, handler: Action1<any>): void;
    removeEventListener(type: "message", handler: Action1<MessageEventLike<T>>): void;
    removeEventListener(type: string, handler: Action1<any>): void;
}
type Target = {
    (type: "message", handler: Action1<MessageEventLike<any>>): void;
    (type: string, handler: Action1<any>): void;
};

function f1(s: Source, t: Target) {
    t = s;
AssertType(t = s, "Source");
AssertType(t, "Target");
AssertType(s, "Source");
}

