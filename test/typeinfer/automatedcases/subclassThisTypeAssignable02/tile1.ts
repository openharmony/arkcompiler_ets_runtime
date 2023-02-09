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

// === tests/cases/compiler/tile1.ts ===
declare function AssertType(value:any, type:string):void;
interface Lifecycle<Attrs, State extends Lifecycle<Attrs, State>> {
	oninit?(vnode: Vnode<Attrs, State>): number;
	[_: number]: any;
}

interface Vnode<Attrs, State extends Lifecycle<Attrs, State>> {
	tag: Component<Attrs, State>;
}

interface Component<Attrs, State extends Lifecycle<Attrs, State>> {
	view(this: State, vnode: Vnode<Attrs, State>): number;
}

interface ClassComponent<A> extends Lifecycle<A, ClassComponent<A>> {
	oninit?(vnode: Vnode<A, this>): number;
	view(vnode: Vnode<A, this>): number;
}

interface MyAttrs { id: number }
class C implements ClassComponent<MyAttrs> {
	view(v: Vnode<MyAttrs, C>) { 
AssertType(0, "int");
return 0; 
}

	// Must declare a compatible-ish index signature or else
	// we won't correctly implement ClassComponent.
	[_: number]: unknown;
}

const test8: ClassComponent<any> = new C();
AssertType(test8, "ClassComponent<any>");
AssertType(new C(), "C");
AssertType(C, "typeof C");


