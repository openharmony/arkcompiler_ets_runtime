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

#ifndef MAPLEBE_INCLUDE_CG_SECTION_H
#define MAPLEBE_INCLUDE_CG_SECTION_H

#include "ifile.h"
#include "obj_emit.h"

namespace maplebe {
Section::Section(std::string secName, Word type, Word flags, uint32 align, ObjEmitter &objEmitter, MemPool &memPool)
    : emitter(objEmitter), name(secName, &memPool)
{
    secHeader.sh_type = type;
    secHeader.sh_flags = flags;
    secHeader.sh_addralign = align;
    emitter.RegisterSection(this);
}

void Section::Layout()
{
    emitter.UpdateSectionOffsetAddr(this);
    GenerateData();
    HandleLocalFixup();
    emitter.UpdateGlobalOffsetAddr(this);
}

} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_SECTION_H */
