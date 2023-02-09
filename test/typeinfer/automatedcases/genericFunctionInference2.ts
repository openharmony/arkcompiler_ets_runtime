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

// === tests/cases/compiler/genericFunctionInference2.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #30685

type Reducer<S> = (state: S) => S;
declare function combineReducers<S>(reducers: { [K in keyof S]: Reducer<S[K]> }): Reducer<S>;

type MyState = { combined: { foo: number } };
declare const foo: Reducer<MyState['combined']['foo']>;
AssertType(foo, "Reducer<number>");

const myReducer1: Reducer<MyState> = combineReducers({
AssertType(myReducer1, "Reducer<MyState>");
AssertType(combineReducers({    combined: combineReducers({ foo }),}), "Reducer<{ combined: { foo: number; }; }>");
AssertType(combineReducers, "<S>({ [K in keyof S]: Reducer<S[K]>; }) => Reducer<S>");
AssertType({    combined: combineReducers({ foo }),}, "{ combined: Reducer<{ foo: number; }>; }");

    combined: combineReducers({ foo }),
AssertType(combined, "Reducer<{ foo: number; }>");
AssertType(combineReducers({ foo }), "Reducer<{ foo: number; }>");
AssertType(combineReducers, "<S>({ [K in keyof S]: Reducer<S[K]>; }) => Reducer<S>");
AssertType({ foo }, "{ foo: Reducer<number>; }");
AssertType(foo, "Reducer<number>");

});

const myReducer2 = combineReducers({
AssertType(myReducer2, "Reducer<{ combined: { foo: number; }; }>");
AssertType(combineReducers({    combined: combineReducers({ foo }),}), "Reducer<{ combined: { foo: number; }; }>");
AssertType(combineReducers, "<S>({ [K in keyof S]: Reducer<S[K]>; }) => Reducer<S>");
AssertType({    combined: combineReducers({ foo }),}, "{ combined: Reducer<{ foo: number; }>; }");

    combined: combineReducers({ foo }),
AssertType(combined, "Reducer<{ foo: number; }>");
AssertType(combineReducers({ foo }), "Reducer<{ foo: number; }>");
AssertType(combineReducers, "<S>({ [K in keyof S]: Reducer<S[K]>; }) => Reducer<S>");
AssertType({ foo }, "{ foo: Reducer<number>; }");
AssertType(foo, "Reducer<number>");

});

// Repro from #30942

declare function withH<T, U>(handlerCreators: HandleCreatorsFactory<T, U>): U;

type Props = { out: number }

type HandleCreatorsFactory<T, U> = (initialProps: T) => { [P in keyof U]: (props: T) => U[P] };

const enhancer4 = withH((props: Props) => ({
AssertType(enhancer4, "{ onChange: (any) => void; onSubmit: (any) => void; }");
AssertType(withH((props: Props) => ({    onChange: (props) => (e: any) => {},    onSubmit: (props) => (e: any) => {},})), "{ onChange: (any) => void; onSubmit: (any) => void; }");
AssertType(withH, "<T, U>(HandleCreatorsFactory<T, U>) => U");
AssertType((props: Props) => ({    onChange: (props) => (e: any) => {},    onSubmit: (props) => (e: any) => {},}), "(Props) => { onChange: (Props) => (any) => void; onSubmit: (Props) => (any) => void; }");
AssertType(props, "Props");
AssertType(({    onChange: (props) => (e: any) => {},    onSubmit: (props) => (e: any) => {},}), "{ onChange: (Props) => (any) => void; onSubmit: (Props) => (any) => void; }");
AssertType({    onChange: (props) => (e: any) => {},    onSubmit: (props) => (e: any) => {},}, "{ onChange: (Props) => (any) => void; onSubmit: (Props) => (any) => void; }");

    onChange: (props) => (e: any) => {},
AssertType(onChange, "(Props) => (any) => void");
AssertType((props) => (e: any) => {}, "(Props) => (any) => void");
AssertType(props, "Props");
AssertType((e: any) => {}, "(any) => void");
AssertType(e, "any");

    onSubmit: (props) => (e: any) => {},
AssertType(onSubmit, "(Props) => (any) => void");
AssertType((props) => (e: any) => {}, "(Props) => (any) => void");
AssertType(props, "Props");
AssertType((e: any) => {}, "(any) => void");
AssertType(e, "any");

}));

enhancer4.onChange(null);
AssertType(enhancer4.onChange(null), "void");
AssertType(enhancer4.onChange, "(any) => void");
AssertType(null, "null");


