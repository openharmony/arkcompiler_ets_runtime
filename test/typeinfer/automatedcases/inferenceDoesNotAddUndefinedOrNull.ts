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

// === tests/cases/compiler/inferenceDoesNotAddUndefinedOrNull.ts ===
declare function AssertType(value:any, type:string):void;
interface NodeArray<T extends Node> extends ReadonlyArray<T> {}

interface Node {
    forEachChild<T>(cbNode: (node: Node) => T | undefined, cbNodeArray?: (nodes: NodeArray<Node>) => T | undefined): T | undefined;
}

declare function toArray<T>(value: T | T[]): T[];
declare function toArray<T>(value: T | readonly T[]): readonly T[];

function flatMapChildren<T>(node: Node, cb: (child: Node) => readonly T[] | T | undefined): readonly T[] {
    const result: T[] = [];
AssertType(result, "T[]");
AssertType([], "never[]");

    node.forEachChild(child => {
AssertType(node.forEachChild(child => {        const value = cb(child);        if (value !== undefined) {            result.push(...toArray(value));        }    }), "union");
AssertType(node.forEachChild, "<T>((Node) => union, ?union) => union");
AssertType(child => {        const value = cb(child);        if (value !== undefined) {            result.push(...toArray(value));        }    }, "(Node) => void");
AssertType(child, "Node");

        const value = cb(child);
AssertType(value, "union");
AssertType(cb(child), "union");
AssertType(cb, "(Node) => union");
AssertType(child, "Node");

        if (value !== undefined) {
AssertType(value !== undefined, "boolean");
AssertType(value, "union");
AssertType(undefined, "undefined");

            result.push(...toArray(value));
AssertType(result.push(...toArray(value)), "number");
AssertType(result.push, "(...T[]) => number");
AssertType(...toArray(value), "T");
AssertType(toArray(value), "readonly T[]");
AssertType(toArray, "{ <T>(union): T[]; <T>(union): readonly T[]; }");
AssertType(value, "union");
        }
    });
AssertType(result, "T[]");
    return result;
}

function flatMapChildren2<T>(node: Node, cb: (child: Node) => readonly T[] | T | null): readonly T[] {
    const result: T[] = [];
AssertType(result, "T[]");
AssertType([], "never[]");

    node.forEachChild(child => {
AssertType(node.forEachChild(child => {        const value = cb(child);        if (value !== null) {            result.push(...toArray(value));        }    }), "union");
AssertType(node.forEachChild, "<T>((Node) => union, ?union) => union");
AssertType(child => {        const value = cb(child);        if (value !== null) {            result.push(...toArray(value));        }    }, "(Node) => void");
AssertType(child, "Node");

        const value = cb(child);
AssertType(value, "union");
AssertType(cb(child), "union");
AssertType(cb, "(Node) => union");
AssertType(child, "Node");

        if (value !== null) {
AssertType(value !== null, "boolean");
AssertType(value, "union");
AssertType(null, "null");

            result.push(...toArray(value));
AssertType(result.push(...toArray(value)), "number");
AssertType(result.push, "(...T[]) => number");
AssertType(...toArray(value), "T");
AssertType(toArray(value), "readonly T[]");
AssertType(toArray, "{ <T>(union): T[]; <T>(union): readonly T[]; }");
AssertType(value, "union");
        }
    });
AssertType(result, "T[]");
    return result;
}


