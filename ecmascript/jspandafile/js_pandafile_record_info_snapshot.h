/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_RECORD_INFO_SNAPSHOT_H
#define ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_RECORD_INFO_SNAPSHOT_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/jspandafile/js_pandafile.h"

namespace panda::ecmascript {
/**
 * JSPandaFileRecordInfoSnapshot persists InitializeMergedPF results to avoid page faults during cold start.
 *
 * The recordInfo is stored in the js_pandafile_snapshot's _Pandafile.ams.
 * It contains:
 * - numClasses_, numMethods_, isBundlePack_
 * - jsRecordInfo_ (with owned strings to avoid string_view dangling)
 * - npmEntries_ (with owned strings)
 */
class FileMemMapWriter;
class FileMemMapReader;

class JSPandaFileRecordInfoSnapshot {
public:
    // Interfaces for integration with JSPandaFileSnapshot
    // Calculate the size needed for recordInfo section in combined snapshot
    static size_t CalculateRecordInfoSectionSize(const JSPandaFile *jsPandaFile);
    // Write recordInfo section to the snapshot writer
    static bool WriteRecordInfoSection(const EcmaVM *vm, const JSPandaFile *jsPandaFile, FileMemMapWriter &writer);
    // Read recordInfo section from the snapshot reader
    static bool ReadRecordInfoSection(JSPandaFile *jsPandaFile, FileMemMapReader &reader);
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_JSPANDAFILE_JS_PANDAFILE_RECORD_INFO_SNAPSHOT_H
