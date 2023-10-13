/*
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
 */
class SubscribeEvent {
    private static subscriber = null;
    private static callback?:(event:string) => void;

    static createSubscriber(subscribeInfo, callback:(event:string) => void) {
        print("createSubscriber");
    }

    static subscribe(subscriber) {
        print("subscribe");
    }

    static unSubscribe() {
        print("unSubscribe");
    }
}
new SubscribeEvent();
let a : (event:string) => void = function() : void{};
SubscribeEvent.createSubscriber("", a);
SubscribeEvent.subscribe("");
SubscribeEvent.unSubscribe();
print(1);
