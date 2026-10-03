// Provide explicit instantiations for LLVM template classes that are missing
// from the static library builds. The extern template declarations in headers
// suppress implicit instantiation but the explicit instantiation definitions
// are absent from the .lib files.
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/FormatVariadic.h"
#include <string>

namespace llvm {

// SmallVectorBase explicit instantiations
template class SmallVectorBase<uint32_t>;
#if SIZE_MAX > UINT32_MAX
template class SmallVectorBase<uint64_t>;
#endif

// cl::basic_parser explicit instantiations
template class cl::basic_parser<std::string>;

}
