// Copyright 2021 The VGC Developers
// See the COPYRIGHT file at the top-level directory of this distribution
// and at https://github.com/vgc/vgc/blob/master/COPYRIGHT
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <vgc/graphics/idgenerator.h>

// Note: we need this .cpp file because even though it doesn't define any
// function, it includes the above .h file, informing the compiler about the
// IdGenerator symbol which must be exported (despite the class being
// all-inline). Not having this *.cpp file results in a linker error on
// MSVC when trying to instanciate an IdGenerator from another DLL.
