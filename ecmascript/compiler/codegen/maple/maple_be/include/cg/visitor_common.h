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

#ifndef MAPLEBE_INCLUDE_CG_VISITOR_COMMON_H
#define MAPLEBE_INCLUDE_CG_VISITOR_COMMON_H
namespace maplebe {
class OperandVisitorBase {
public:
    virtual ~OperandVisitorBase() = default;
};

template <typename Visitable>
class OperandVisitor {
public:
    virtual ~OperandVisitor() = default;
    virtual void Visit(Visitable *v) = 0;
};

template <typename... V>
class OperandVisitors {
public:
    virtual ~OperandVisitors() = default;
};

template <typename OpV1, typename OpV2, typename... OpV3>
class OperandVisitors<OpV1, OpV2, OpV3...>
    : public OperandVisitor<OpV1>, public OperandVisitor<OpV2>, public OperandVisitor<OpV3>... {
};
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_VISITOR_COMMON_H */
