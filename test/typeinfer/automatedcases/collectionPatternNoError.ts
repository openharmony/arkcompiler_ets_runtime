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

// === tests/cases/compiler/collectionPatternNoError.ts ===
declare function AssertType(value:any, type:string):void;
interface MsgConstructor<T extends Message> {
  new(data: Array<{}>): T;
}
class Message {
  clone(): this {
AssertType(this, "this");
    return this;
  }
}
interface MessageList<T extends Message> extends Message {
  methodOnMessageList(): T[];
}

function fetchMsg<V extends Message>(protoCtor: MsgConstructor<V>): V {
AssertType(null!, "null");
AssertType(null, "null");
  return null!;
}

class DataProvider<T extends Message, U extends MessageList<T>> {
  constructor(
    private readonly message: MsgConstructor<T>,
    private readonly messageList: MsgConstructor<U>,
  ) { }

  fetch() {
    const messageList = fetchMsg(this.messageList);
AssertType(messageList, "U");
AssertType(fetchMsg(this.messageList), "U");
AssertType(fetchMsg, "<V extends Message>(MsgConstructor<V>) => V");
AssertType(this.messageList, "MsgConstructor<U>");
AssertType(this, "this");

    messageList.methodOnMessageList();
AssertType(messageList.methodOnMessageList(), "T[]");
AssertType(messageList.methodOnMessageList, "() => T[]");
  }
}

// The same bug as the above but using indexed accesses
// (won't surface directly unless unsound indexed access assignments are forbidden)
function f<
  U extends {TType: MessageList<T>},
  T extends Message
>(message: MsgConstructor<T>, messageList: MsgConstructor<U["TType"]>) {
  fetchMsg(messageList).methodOnMessageList();
AssertType(fetchMsg(messageList).methodOnMessageList(), "T[]");
AssertType(fetchMsg(messageList).methodOnMessageList, "() => T[]");
AssertType(fetchMsg(messageList), "U["TType"]");
AssertType(fetchMsg, "<V extends Message>(MsgConstructor<V>) => V");
AssertType(messageList, "MsgConstructor<U["TType"]>");
}


