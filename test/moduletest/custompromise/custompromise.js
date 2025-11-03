/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 * @tc.name:custompromise
 * @tc.desc:test custompromise
 * @tc.type: FUNC
 * @tc.require: issue11740
 */
class MyPromise extends Promise {
  constructor(executor) {
    super(executor);
  }
  then(onfulfilled, onRejected) {
    print("custom then");
    return super.then(onfulfilled, onRejected);
  }
  catch(onRejected) {
    print("custom catch");
    return super.catch(onRejected);
  }
  finally(onFinally) {
    print("custom finally");
    return super.finally(onFinally);
  }
}

new MyPromise((resolve) => {
  resolve();
}).then(() => {
  print("then resolve");
});


new MyPromise((resolve, reject) => {
  reject();
}).then(() => {}, () => {
  print("then reject");
  throw new Error("then throw");
}).catch((e) => {
  print(e);
}).finally(() => {
  print("finally");
})
