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

#include "ecmascript/js_symbol.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSSymbolTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(JSSymbolTest, SymbolCreate)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSymbol> normallSymbol = factory->NewJSSymbol();
    EXPECT_TRUE(*normallSymbol != nullptr);
    EXPECT_TRUE(normallSymbol->GetDescription().IsUndefined());

    JSHandle<JSSymbol> privateSymbol = factory->NewPrivateSymbol();
    EXPECT_TRUE(*privateSymbol != nullptr);
    EXPECT_TRUE(privateSymbol->IsPrivate());
    EXPECT_TRUE(privateSymbol->GetDescription().IsUndefined());

    JSHandle<JSTaggedValue> symbolName(factory->NewFromASCII("hello world"));
    JSHandle<JSSymbol> privateNameSymbol = factory->NewPrivateNameSymbol(symbolName);
    EXPECT_TRUE(*privateNameSymbol != nullptr);
    EXPECT_TRUE(privateNameSymbol->IsPrivateNameSymbol());
    EXPECT_FALSE(privateNameSymbol->GetDescription().IsUndefined());

    JSHandle<JSSymbol> wellKnowSymbol = factory->NewWellKnownSymbol(symbolName);
    EXPECT_TRUE(*wellKnowSymbol != nullptr);
    EXPECT_TRUE(wellKnowSymbol->IsWellKnownSymbol());
    EXPECT_FALSE(wellKnowSymbol->GetDescription().IsUndefined());
}

HWTEST_F_L0(JSSymbolTest, SymbolEqual)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> helloWord(factory->NewFromASCII("hello world"));
    JSHandle<JSTaggedValue> hiWord(factory->NewFromASCII("hi world"));

    JSHandle<JSTaggedValue> helloWord1SymbolVal(factory->NewPrivateNameSymbol(helloWord));
    JSHandle<JSTaggedValue> helloWord2SymbolVal(factory->NewPrivateNameSymbol(helloWord));
    JSHandle<JSTaggedValue> hiWordSymbolVal(factory->NewPrivateNameSymbol(hiWord));
    JSSymbol *helloWord1Symbol = JSSymbol::Cast(helloWord1SymbolVal->GetTaggedObject());
    JSSymbol *helloWord2Symbol = JSSymbol::Cast(helloWord2SymbolVal->GetTaggedObject());
    JSSymbol *hiWordSymbol = JSSymbol::Cast(hiWordSymbolVal->GetTaggedObject());
    EXPECT_TRUE(JSSymbol::Equal(*helloWord1Symbol, *helloWord2Symbol));
    helloWord2Symbol->SetFlags(1);
    EXPECT_FALSE(JSSymbol::Equal(*helloWord1Symbol, *helloWord2Symbol));
    EXPECT_FALSE(JSSymbol::Equal(*helloWord1Symbol, *hiWordSymbol));
}
}  // namespace panda::test
