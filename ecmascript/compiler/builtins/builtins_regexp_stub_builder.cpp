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

#include "ecmascript/compiler/builtins/builtins_regexp_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/regexp/regexp_parser.h"
namespace panda::ecmascript::kungfu {

void BuiltinsRegExpStubBuilder::GetFlags(GateRef glue, GateRef thisValue,
    [[maybe_unused]] GateRef numArgs, Variable* result, Label* exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isEcmaObject(env);
    BRANCH(IsEcmaObject(glue, thisValue), &isEcmaObject, slowPath);
    Bind(&isEcmaObject);
    Label fastRegExpPath(env);
    GateRef fastRegExp = CallNGCRuntime(glue, RTSTUB_ID(IsFastRegExp), {glue, thisValue});
    BRANCH(fastRegExp, slowPath, &fastRegExpPath);
    Bind(&fastRegExpPath);
    {
        Label hasException(env);
        DEFVARIABLE(bitFlagsStr, VariableType::INT32(), Int32(0));
        GateRef globalResult = TryToFastGetProperty(glue, thisValue, ConstantIndex::GLOBAL_STRING_INDEX);
        Label notHasException(env);
        BRANCH(HasPendingException(glue), &hasException, &notHasException);
        Bind(&notHasException);
        Label globalKey(env);
        Label notGlobalKey(env);
        BRANCH(TaggedIsTrue(globalResult), &globalKey, &notGlobalKey);
        Bind(&globalKey);
        {
            bitFlagsStr = Int32Or(*bitFlagsStr, Int32(RegExpParser::FLAG_GLOBAL));
            Jump(&notGlobalKey);
        }
        Bind(&notGlobalKey);
        GateRef ignoreCaseResult = TryToFastGetProperty(glue, thisValue, ConstantIndex::IGNORE_CASE_INDEX);
        Label notHasException1(env);
        BRANCH(HasPendingException(glue), &hasException, &notHasException1);
        Bind(&notHasException1);
        Label ignoreCaseKey(env);
        Label notIgnoreCaseKey(env);
        BRANCH(TaggedIsTrue(ignoreCaseResult), &ignoreCaseKey, &notIgnoreCaseKey);
        Bind(&ignoreCaseKey);
        {
            bitFlagsStr = Int32Or(*bitFlagsStr, Int32(RegExpParser::FLAG_IGNORECASE));
            Jump(&notIgnoreCaseKey);
        }
        Bind(&notIgnoreCaseKey);
        GateRef multilineResult = TryToFastGetProperty(glue, thisValue, ConstantIndex::MULTILINE_INDEX);
        Label notHasException2(env);
        BRANCH(HasPendingException(glue), &hasException, &notHasException2);
        Bind(&notHasException2);
        Label multilineKey(env);
        Label notMultilineKey(env);
        BRANCH(TaggedIsTrue(multilineResult), &multilineKey, &notMultilineKey);
        Bind(&multilineKey);
        {
            bitFlagsStr = Int32Or(*bitFlagsStr, Int32(RegExpParser::FLAG_MULTILINE));
            Jump(&notMultilineKey);
        }
        Bind(&notMultilineKey);
        GateRef dotAllResult = TryToFastGetProperty(glue, thisValue, ConstantIndex::DOT_ALL_INDEX);
        Label notHasException3(env);
        BRANCH(HasPendingException(glue), &hasException, &notHasException3);
        Bind(&notHasException3);
        Label dotAllKey(env);
        Label notDotAllKey(env);
        BRANCH(TaggedIsTrue(dotAllResult), &dotAllKey, &notDotAllKey);
        Bind(&dotAllKey);
        {
            bitFlagsStr = Int32Or(*bitFlagsStr, Int32(RegExpParser::FLAG_DOTALL));
            Jump(&notDotAllKey);
        }
        Bind(&notDotAllKey);
        GateRef unicodeResult = TryToFastGetProperty(glue, thisValue, ConstantIndex::UNICODE_INDEX);
        Label notHasException4(env);
        BRANCH(HasPendingException(glue), &hasException, &notHasException4);
        Bind(&notHasException4);
        Label unicodeKey(env);
        Label notUnicodeKey(env);
        BRANCH(TaggedIsTrue(unicodeResult), &unicodeKey, &notUnicodeKey);
        Bind(&unicodeKey);
        {
            bitFlagsStr = Int32Or(*bitFlagsStr, Int32(RegExpParser::FLAG_UTF16));
            Jump(&notUnicodeKey);
        }
        Bind(&notUnicodeKey);
        GateRef stickyResult = TryToFastGetProperty(glue, thisValue, ConstantIndex::STICKY_INDEX);
        Label notHasException5(env);
        BRANCH(HasPendingException(glue), &hasException, &notHasException5);
        Bind(&notHasException5);
        Label stickyKey(env);
        Label notStickyKey(env);
        BRANCH(TaggedIsTrue(stickyResult), &stickyKey, &notStickyKey);
        Bind(&stickyKey);
        {
            bitFlagsStr = Int32Or(*bitFlagsStr, Int32(RegExpParser::FLAG_STICKY));
            Jump(&notStickyKey);
        }
        Bind(&notStickyKey);
        GateRef indicesResult = TryToFastGetProperty(glue, thisValue, ConstantIndex::HAS_INDICES_INDEX);
        Label notHasException6(env);
        BRANCH(HasPendingException(glue), &hasException, &notHasException6);
        Bind(&notHasException6);
        Label hasIndicesKey(env);
        Label notHasIndicesKey(env);
        BRANCH(TaggedIsTrue(indicesResult), &hasIndicesKey, &notHasIndicesKey);
        Bind(&hasIndicesKey);
        {
            bitFlagsStr = Int32Or(*bitFlagsStr, Int32(RegExpParser::FLAG_HASINDICES));
            Jump(&notHasIndicesKey);
        }
        Bind(&notHasIndicesKey);
        GateRef flagsStr = CallRuntime(glue, RTSTUB_ID(GetAllFlagsInternal), { IntToTaggedInt(*bitFlagsStr)});
        result->WriteVariable(flagsStr);
        Jump(exit);
        Bind(&hasException);
        {
            result->WriteVariable(GetGlobalConstantValue(VariableType::JS_POINTER(),
                glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX));
            Jump(exit);
        }
    }
}

GateRef BuiltinsRegExpStubBuilder::TryToFastGetProperty(GateRef glue, GateRef thisValue, ConstantIndex constIndex)
{
    auto env = GetEnvironment();
    Label entry(env);
    Label exit(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());
    GateRef key = GetGlobalConstantValue(VariableType::JS_POINTER(), glue, constIndex);
    GateRef property = FastGetPropertyByName(glue, thisValue, key, ProfileOperation());
    Label hasException(env);
    Label notHasException(env);
    BRANCH(HasPendingException(glue), &hasException, &notHasException);
    Bind(&hasException);
    {
        result = Exception();
        Jump(&exit);
    }
    Bind(&notHasException);
    {
        result = FastToBoolean(glue, property);
        Jump(&exit);
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}
}  // namespace panda::ecmascript::kungfu
