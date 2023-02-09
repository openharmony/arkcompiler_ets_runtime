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

// === tests/cases/compiler/observableInferenceCanBeMade.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #33131

declare function of<T>(a: T): Observable<T>;
declare function from<O extends ObservableInput<any>>(input: O): Observable<ObservedValueOf<O>>;

type ObservedValueOf<O> = O extends ObservableInput<infer T> ? T : never;

interface Subscribable<T> {
    subscribe(next?: (value: T) => void, error?: (error: any) => void, complete?: () => void): void;
}
type ObservableInput<T> = Subscribable<T> | Subscribable<never>;


declare class Observable<T> implements Subscribable<T> {
    subscribe(next?: (value: T) => void, error?: (error: any) => void, complete?: () => void): void;
}

function asObservable(input: string | ObservableInput<string>): Observable<string> {
AssertType(typeof input === 'string' ? of(input) : from(input), "Observable<string>");
AssertType(typeof input === 'string', "boolean");
AssertType(typeof input, "union");
AssertType(input, "union");
AssertType('string', "string");
AssertType(of(input), "Observable<string>");
AssertType(of, "<T>(T) => Observable<T>");
AssertType(input, "string");
AssertType(from(input), "Observable<string>");
AssertType(from, "<O extends ObservableInput<any>>(O) => Observable<ObservedValueOf<O>>");
AssertType(input, "ObservableInput<string>");
    return typeof input === 'string' ? of(input) : from(input)
}


