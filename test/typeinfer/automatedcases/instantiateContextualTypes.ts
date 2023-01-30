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

// === tests/cases/compiler/instantiateContextualTypes.ts ===
declare function AssertType(value:any, type:string):void;
// #6611

export interface A<a> {
    value: a;
}

function fn<a>(values: A<a>, value: a) : void {
}

declare let handlers: A<(value: number) => void>;
AssertType(handlers, "A<(number) => void>");
AssertType(value, "number");

fn(handlers, value => alert(value));
AssertType(fn(handlers, value => alert(value)), "void");
AssertType(fn, "<a>(A<a>, a) => void");
AssertType(handlers, "A<(number) => void>");
AssertType(value => alert(value), "(number) => void");
AssertType(value, "number");
AssertType(alert(value), "void");
AssertType(alert, "(?any) => void");
AssertType(value, "number");

// #21382

interface BaseProps<T> {
  initialValues: T;
  nextValues: (cur: T) => T;
}
declare class Component<P> { constructor(props: P); props: P; }
declare class GenericComponent<Props = {}, Values = object>
    extends Component<Props & BaseProps<Values>> {
  iv: Values;
}

new GenericComponent({ initialValues: 12, nextValues: val => 12 });
AssertType(new GenericComponent({ initialValues: 12, nextValues: val => 12 }), "GenericComponent<{ initialValues: number; nextValues: (number) => number; }, number>");
AssertType(GenericComponent, "typeof GenericComponent");
AssertType({ initialValues: 12, nextValues: val => 12 }, "{ initialValues: number; nextValues: (number) => number; }");
AssertType(initialValues, "number");
AssertType(12, "int");
AssertType(nextValues, "(number) => number");
AssertType(val => 12, "(number) => number");
AssertType(val, "number");
AssertType(12, "int");

// #22149

declare function useStringOrNumber<T extends string | number>(t: T, useIt: T extends string ? ((s: string) => void) : ((n: number) => void)): void;
useStringOrNumber("", foo => {});
AssertType(useStringOrNumber("", foo => {}), "void");
AssertType(useStringOrNumber, "<T extends union>(T, T extends string ? (string) => void : (number) => void) => void");
AssertType("", "string");
AssertType(foo => {}, "(string) => void");
AssertType(foo, "string");

// #25299

type ActionType<P> = string & { attachPayloadTypeHack?: P & never }

type Handler<S, P> = P extends void
    ? (state: S) => S
    : (state: S, payload: P) => S

interface ActionHandler<S, P> {
    actionType: ActionType<P>
    handler: Handler<S, P>
}

declare function handler<S, P>(actionType: ActionType<P>, handler: Handler<S, P>): ActionHandler<S, P>

declare function createReducer<S>(
        defaultState: S,
        ...actionHandlers: ActionHandler<S, any>[]
    ): any

interface AppState {
    dummy: string
}

const defaultState: AppState = {
AssertType(defaultState, "AppState");
AssertType({    dummy: ''}, "{ dummy: string; }");

    dummy: ''
AssertType(dummy, "string");
AssertType('', "string");
}

const NON_VOID_ACTION: ActionType<number> = 'NON_VOID_ACTION'
AssertType(NON_VOID_ACTION, "ActionType<number>");
AssertType('NON_VOID_ACTION', "string");

    , VOID_ACTION: ActionType<void> = 'VOID_ACTION'
AssertType(VOID_ACTION, "ActionType<void>");
AssertType('VOID_ACTION', "string");

createReducer(
AssertType(createReducer(    defaultState,    handler(NON_VOID_ACTION, (state, _payload) => state),    handler(VOID_ACTION, state => state)), "any");
AssertType(createReducer, "<S>(S, ...ActionHandler<S, any>[]) => any");

    defaultState,
AssertType(defaultState, "AppState");

    handler(NON_VOID_ACTION, (state, _payload) => state),
AssertType(handler(NON_VOID_ACTION, (state, _payload) => state), "ActionHandler<AppState, number>");
AssertType(handler, "<S, P>(ActionType<P>, Handler<S, P>) => ActionHandler<S, P>");
AssertType(NON_VOID_ACTION, "ActionType<number>");
AssertType((state, _payload) => state, "(AppState, number) => AppState");
AssertType(state, "AppState");
AssertType(_payload, "number");
AssertType(state, "AppState");

    handler(VOID_ACTION, state => state)
AssertType(handler(VOID_ACTION, state => state), "ActionHandler<AppState, void>");
AssertType(handler, "<S, P>(ActionType<P>, Handler<S, P>) => ActionHandler<S, P>");
AssertType(VOID_ACTION, "ActionType<void>");
AssertType(state => state, "(AppState) => AppState");
AssertType(state, "AppState");
AssertType(state, "AppState");

)

// #25814

type R = {
  a: (x: number) => void;
  b: (x: string) => void;
};

type O = {
  on<P extends keyof R>(x: P, callback: R[P]): void;
};

declare let x: O;
AssertType(x, "O");

x.on('a', a => {});
AssertType(x.on('a', a => {}), "void");
AssertType(x.on, "<P extends keyof R>(P, R[P]) => void");
AssertType('a', "string");
AssertType(a => {}, "(number) => void");
AssertType(a, "number");

// #29775

namespace N1 {

declare class Component<P> { 
  constructor(props: P);
}
    
interface ComponentClass<P = {}> {
  new (props: P): Component<P>;
}

type CreateElementChildren<P> =
  P extends { children?: infer C }
  ? C extends any[]
    ? C
    : C[]
  : unknown;

declare function createElement<P extends {}>(
  type: ComponentClass<P>,
  ...children: CreateElementChildren<P>
): any;

declare function createElement2<P extends {}>(
  type: ComponentClass<P>,
  child: CreateElementChildren<P>
): any;

class InferFunctionTypes extends Component<{children: (foo: number) => string}> {}

createElement(InferFunctionTypes, (foo) => "" + foo);

createElement2(InferFunctionTypes, [(foo) => "" + foo]);

}

// #30341

type InnerBox<T> = {
  value: T;
}

type OuterBox<T> = {
  inner: InnerBox<T>
};

type BoxConsumerFromOuterBox<T> =
  T extends OuterBox<infer U> ?
      (box: InnerBox<U>) => void :
      never;

declare function passContentsToFunc<T>(outerBox: T, consumer: BoxConsumerFromOuterBox<T>): void;

declare const outerBoxOfString: OuterBox<string>;
AssertType(outerBoxOfString, "OuterBox<string>");

passContentsToFunc(outerBoxOfString, box => box.value);
AssertType(passContentsToFunc(outerBoxOfString, box => box.value), "void");
AssertType(passContentsToFunc, "<T>(T, BoxConsumerFromOuterBox<T>) => void");
AssertType(outerBoxOfString, "OuterBox<string>");
AssertType(box => box.value, "(InnerBox<string>) => string");
AssertType(box, "InnerBox<string>");
AssertType(box.value, "string");

// Repro from #32349

type DooDad = 'SOMETHING' | 'ELSE' ;

class Interesting {
	public compiles = () : Promise<DooDad> => {
AssertType(Promise.resolve().then(() => {			if (1 < 2) {				return 'SOMETHING';			}			return 'ELSE';		}), "Promise<union>");
AssertType(Promise.resolve().then, "<TResult1 = void, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(Promise.resolve(), "Promise<void>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(() => {			if (1 < 2) {				return 'SOMETHING';			}			return 'ELSE';		}, "() => union");
		return Promise.resolve().then(() => {

			if (1 < 2) {
AssertType(1 < 2, "boolean");
AssertType(1, "int");
AssertType(2, "int");

AssertType('SOMETHING', "string");
				return 'SOMETHING';
			}
AssertType('ELSE', "string");
			return 'ELSE';

		});
	};
	public doesnt = () : Promise<DooDad> => {
AssertType(Promise.resolve().then(() => {			return 'ELSE';		}), "Promise<union>");
AssertType(Promise.resolve().then, "<TResult1 = void, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(Promise.resolve(), "Promise<void>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(() => {			return 'ELSE';		}, "() => "ELSE"");
		return Promise.resolve().then(() => {

AssertType('ELSE', "string");
			return 'ELSE';

		});
	};
	public slightlyDifferentErrorMessage = () : Promise<DooDad> => {
AssertType(Promise.resolve().then(() => {			if (1 < 2) {				return 'SOMETHING';			}			return 'SOMETHING';		}), "Promise<union>");
AssertType(Promise.resolve().then, "<TResult1 = void, TResult2 = never>(?union, ?union) => Promise<union>");
AssertType(Promise.resolve(), "Promise<void>");
AssertType(Promise.resolve, "{ (): Promise<void>; <T>(T): Promise<Awaited<T>>; <T>(union): Promise<Awaited<T>>; }");
AssertType(() => {			if (1 < 2) {				return 'SOMETHING';			}			return 'SOMETHING';		}, "() => "SOMETHING"");
		return Promise.resolve().then(() => {

			if (1 < 2) {
AssertType(1 < 2, "boolean");
AssertType(1, "int");
AssertType(2, "int");

AssertType('SOMETHING', "string");
				return 'SOMETHING';
			}
AssertType('SOMETHING', "string");
			return 'SOMETHING';

		});
	};
}

// Repro from #32349

declare function invoke<T>(f: () => T): T;

let xx: 0 | 1 | 2 = invoke(() => 1);
AssertType(xx, "union");
AssertType(invoke(() => 1), "int");
AssertType(invoke, "<T>(() => T) => T");
AssertType(() => 1, "() => 1");
AssertType(1, "int");

// Repro from #32416

declare function assignPartial<T>(target: T, partial: Partial<T>): T;

let obj = {
AssertType(obj, "{ foo(string): void; }");
AssertType({  foo(bar: string) {}}, "{ foo(string): void; }");

  foo(bar: string) {
AssertType(foo, "(string) => void");

AssertType(bar, "string");
}
}

assignPartial(obj, { foo(...args) {} });  // args has type [string]
AssertType(assignPartial(obj, { foo(...args) {} }), "{ foo(string): void; }");
AssertType(assignPartial, "<T>(T, Partial<T>) => T");
AssertType(obj, "{ foo(string): void; }");
AssertType({ foo(...args) {} }, "{ foo(string): void; }");
AssertType(foo, "(string) => void");
AssertType(args, "[bar: string]");


