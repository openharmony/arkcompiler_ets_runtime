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

// === tests/cases/compiler/file1.ts ===
declare function AssertType(value:any, type:string):void;
"use strict";
AssertType("use strict", "string");

import * as MyModule from "./mymodule";

export class BrokenClass {

  constructor() {}

  public brokenMethod(field: string, value: string) {
AssertType(new Promise<Array<MyModule.MyModel>>((resolve, reject) => {    let result: Array<MyModule.MyModel> = [];    let populateItems = (order) => {      return new Promise((resolve, reject) => {        this.doStuff(order.id)          .then((items) => {            order.items = items;            resolve(order);          });      });    };    return Promise.all(result.map(populateItems))      .then((orders: Array<MyModule.MyModel>) => {        resolve(orders);      });    }), "Promise<MyModule.MyModel[]>");
AssertType(Promise, "PromiseConstructor");
AssertType(MyModule, "any");
AssertType((resolve, reject) => {    let result: Array<MyModule.MyModel> = [];    let populateItems = (order) => {      return new Promise((resolve, reject) => {        this.doStuff(order.id)          .then((items) => {            order.items = items;            resolve(order);          });      });    };    return Promise.all(result.map(populateItems))      .then((orders: Array<MyModule.MyModel>) => {        resolve(orders);      });    }, "((union) => void, (?any) => void) => Promise<void>");
AssertType(resolve, "(union) => void");
AssertType(reject, "(?any) => void");
  return new Promise<Array<MyModule.MyModel>>((resolve, reject) => {

    let result: Array<MyModule.MyModel> = [];
AssertType(result, "MyModule.MyModel[]");
AssertType(MyModule, "any");
AssertType([], "undefined[]");

    let populateItems = (order) => {
AssertType(populateItems, "(any) => Promise<unknown>");
AssertType(order, "any");
AssertType((order) => {      return new Promise((resolve, reject) => {        this.doStuff(order.id)          .then((items) => {            order.items = items;            resolve(order);          });      });    }, "(any) => Promise<unknown>");

AssertType(new Promise((resolve, reject) => {        this.doStuff(order.id)          .then((items) => {            order.items = items;            resolve(order);          });      }), "Promise<unknown>");
AssertType(Promise, "PromiseConstructor");
AssertType((resolve, reject) => {        this.doStuff(order.id)          .then((items) => {            order.items = items;            resolve(order);          });      }, "((unknown) => void, (?any) => void) => void");
AssertType(resolve, "(unknown) => void");
AssertType(reject, "(?any) => void");
      return new Promise((resolve, reject) => {

        this.doStuff(order.id)
AssertType(this.doStuff(order.id)          .then((items) => {            order.items = items;            resolve(order);          }), "Promise<void>");
AssertType(this.doStuff(order.id)          .then, "<TResult1 = void, TResult2 = never>(?(void) => union, ?(any) => union) => Promise<union>");
AssertType(this.doStuff(order.id), "Promise<void>");
AssertType(this.doStuff, "(number) => Promise<void>");
AssertType(this, "this");
AssertType(order.id, "any");

          .then((items) => {
AssertType((items) => {            order.items = items;            resolve(order);          }, "(void) => void");
AssertType(items, "void");

            order.items = items;
AssertType(order.items = items, "void");
AssertType(order.items, "any");
AssertType(items, "void");

            resolve(order);
AssertType(resolve(order), "void");
AssertType(resolve, "(unknown) => void");
AssertType(order, "any");

          });
      });
    };

AssertType(Promise.all(result.map(populateItems))      .then((orders: Array<MyModule.MyModel>) => {        resolve(orders);      }), "Promise<void>");
AssertType(Promise.all(result.map(populateItems))      .then, "<TResult1 = unknown[], TResult2 = never>(?(unknown[]) => union, ?(any) => union) => Promise<union>");
AssertType(Promise.all(result.map(populateItems)), "Promise<unknown[]>");
AssertType(Promise.all, "{ <T>(Iterable<union>): Promise<Awaited<T>[]>; <T extends union>(T): Promise<{ -readonly [P in keyof T]: Awaited<T[P]>; }>; }");
AssertType(result.map(populateItems), "Promise<unknown>[]");
AssertType(result.map, "<U>((MyModule.MyModel, number, MyModule.MyModel[]) => U, ?any) => U[]");
AssertType(populateItems, "(any) => Promise<unknown>");
    return Promise.all(result.map(populateItems))

      .then((orders: Array<MyModule.MyModel>) => {
AssertType((orders: Array<MyModule.MyModel>) => {        resolve(orders);      }, "(Array<MyModule.MyModel>) => void");
AssertType(orders, "MyModule.MyModel[]");
AssertType(MyModule, "any");

        resolve(orders);
AssertType(resolve(orders), "void");
AssertType(resolve, "(union) => void");
AssertType(orders, "MyModule.MyModel[]");

      });
    });
  }

  public async doStuff(id: number) {
    return;
  }
}


