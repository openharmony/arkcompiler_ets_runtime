/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

var AsyncGeneratorFunction = Object.getPrototypeOf(async function* () {}).constructor;
print(Object.isExtensible(AsyncGeneratorFunction));

function fun1(){
    var returnGets_1 = 0;
    var asyncIterable_1 = {
      [Symbol.asyncIterator]: function() {
        return this;
      },
      next: function() {
        return {value: 1, done: false};
      },
      get return() {
        returnGets_1 += 1;
        return null;
      },
    };
    
    async function* asyncGenerator() {
      yield* asyncIterable_1;
    }
    
    var asyncIterator_1 = asyncGenerator();
    asyncIterator_1.next().then(function() {
      return asyncIterator_1.return(2).then(function(result) {
        print(result.value);
        print(result.done);
        print(returnGets_1);
      });
    });
}
fun1();

function fun2(){
    var asyncIterable_2 = {
        [Symbol.asyncIterator]: function() {
          return this;
        },
        next: function() {
          return {value: 1, done: false};
        },
      };
      
      async function* asyncGenerator() {
        yield* asyncIterable_2;
      }
      
      var asyncIterator_2 = asyncGenerator();
      asyncIterator_2.next().then(function() {
        var promise = Promise.resolve(2).then(() => 3);
        return asyncIterator_2.return(promise).then(function(result) {
          print(result.value);
          print(result.done);
        });
      });
}
fun2();

function fun3(){
    var token_1 = {};

    var asyncIter_3 = {
    [Symbol.asyncIterator]() {
        return this;
    },
    next() {
        return {
        done: false,
        value: undefined,
        };
    },
    return() {
        return {
        done: false,
        get value() {
            throw token_1;
        }
        };
    }
    };

    async function* f_3() {
    var thrown_3;
    try {
        yield* asyncIter_3;
    } catch (e) {
        thrown_3 = e;
    }
    return thrown_3;
    }

    var iter_3 = f_3();

    iter_3.next().then(() => {
        iter_3.return().then(({value}) => {
        print(value);
    });
    });
}
fun3();