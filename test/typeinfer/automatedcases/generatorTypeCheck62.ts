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

// === tests/cases/conformance/es6/yieldExpressions/generatorTypeCheck62.ts ===
declare function AssertType(value:any, type:string):void;
export interface StrategicState {
    lastStrategyApplied?: string;
}

export function strategy<T extends StrategicState>(stratName: string, gen: (a: T) => IterableIterator<T | undefined>): (a: T) => IterableIterator<T | undefined> {
AssertType(function*(state) {        for (const next of gen(state)) {            if (next) {                next.lastStrategyApplied = stratName;            }            yield next;        }    }, "(T) => Generator<T, void, undefined>");
AssertType(state, "T");
    return function*(state) {

        for (const next of gen(state)) {
AssertType(next, "T");
AssertType(gen(state), "IterableIterator<T>");
AssertType(gen, "(T) => IterableIterator<T>");
AssertType(state, "T");

            if (next) {
AssertType(next, "T");

                next.lastStrategyApplied = stratName;
AssertType(next.lastStrategyApplied = stratName, "string");
AssertType(next.lastStrategyApplied, "string");
AssertType(stratName, "string");
            }
            yield next;
AssertType(yield next, "undefined");
AssertType(next, "T");
        }
    }
}

export interface Strategy<T> {
    (a: T): IterableIterator<T | undefined>;
}

export interface State extends StrategicState {
    foo: number;
}

export const Nothing1: Strategy<State> = strategy("Nothing", function*(state: State) {
AssertType(Nothing1, "Strategy<State>");
AssertType(strategy, "<T extends StrategicState>(string, (T) => IterableIterator<T>) => (T) => IterableIterator<T>");
AssertType("Nothing", "string");
AssertType(function*(state: State) {    return state;}, "(State) => Generator<never, State, undefined>");
AssertType(state, "State");
AssertType(strategy("Nothing", function*(state: State) {    return state;}), "(State) => IterableIterator<State>");

AssertType(state, "State");
    return state;

});

export const Nothing2: Strategy<State> = strategy("Nothing", function*(state: State) {
AssertType(Nothing2, "Strategy<State>");
AssertType(strategy("Nothing", function*(state: State) {    yield state;}), "(State) => IterableIterator<State>");
AssertType(strategy, "<T extends StrategicState>(string, (T) => IterableIterator<T>) => (T) => IterableIterator<T>");
AssertType("Nothing", "string");
AssertType(function*(state: State) {    yield state;}, "(State) => Generator<State, void, undefined>");
AssertType(state, "State");

    yield state;
AssertType(yield state, "undefined");
AssertType(state, "State");

});

export const Nothing3: Strategy<State> = strategy("Nothing", function* (state: State) {
AssertType(Nothing3, "Strategy<State>");
AssertType(strategy, "<T extends StrategicState>(string, (T) => IterableIterator<T>) => (T) => IterableIterator<T>");
AssertType("Nothing", "string");
AssertType(function* (state: State) {    yield ;    return state;}, "(State) => Generator<any, State, undefined>");
AssertType(state, "State");
AssertType(strategy("Nothing", function* (state: State) {    yield ;    return state;}), "(any) => IterableIterator<any>");

    yield ;
AssertType(yield, "undefined");

AssertType(state, "State");
    return state;

});
 

