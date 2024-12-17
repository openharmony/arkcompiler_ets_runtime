/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

/*
 * @tc.name:promiseUseLazyValue
 * @tc.desc:test promiseUseLazyValue
 * @tc.type: FUNC
 * @tc.require: issueIBBVLX
 */
import  lazy { LazyObjectForPromiseResolve } from "./lazyObjectForPromiseResolve"
import  lazy { LazyObjectForPromiseReject } from "./lazyObjectForPromiseReject"
import  lazy { LazyLogOutOfPromise } from "./lazyLogOutOfPromise"
import  lazy { LazyLogForPromiseResolveExecutor } from "./lazyLogForPromiseResolveExecutor"
import  lazy { LazyLogForPromiseRejectExecutor } from "./lazyLogForPromiseRejectExecutor"
import  lazy { LazyLogForPromiseThen } from "./lazyLogForPromiseThen"
import  lazy { LazyLogForPromiseCatch } from "./lazyLogForPromiseCatch"
import  lazy { LazyLogForPromiseResolveFinally, LazyLogForPromiseRejectFinally } from "./lazyLogForPromiseFinally"
import  lazy { lazyGenerator } from "./lazyGenerator"

function promiseFuncResolved() {
  return new Promise((resolve) => {
    LazyLogForPromiseResolveExecutor.log("promiseFuncResolved executor");
    resolve(new LazyObjectForPromiseResolve());
  })
}

function promiseFuncRejected() {
  return new Promise((resolve, reject) => {
    LazyLogForPromiseRejectExecutor.log("promiseFuncRejected executor");
    reject(new LazyObjectForPromiseReject());
  })
}

function promiseUseLazyValue() {
  const iterator = lazyGenerator();
  promiseFuncResolved().then((lazyObj) => {
    LazyLogForPromiseThen.log("promiseFuncResolved then " + iterator.next().value);
    lazyObj.print();
  }).finally(() => {
    LazyLogForPromiseResolveFinally.log("promiseFuncResolved finally " + iterator.next().value)
  })
  promiseFuncRejected().catch((lazyObj) => {
    LazyLogForPromiseCatch.log("promiseFuncRejected catch " + iterator.next().value);
    lazyObj.print();
  }).finally(() => {
    LazyLogForPromiseRejectFinally.log("promiseFuncRejected finally " + iterator.next().value)
  })
  LazyLogOutOfPromise.log("testPromise end " + iterator.next().value);
}

promiseUseLazyValue()