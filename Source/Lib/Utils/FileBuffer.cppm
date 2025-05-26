module;

import LLVM;
import std;

export module FileBuffer;

export namespace chocopy {
class FileBuffer : public llvm::MemoryBuffer {
public:
  using Base = llvm::MemoryBuffer;
  FileBuffer(std::string_view Str) {
    Base::init(Str.data(), Str.data() + Str.size(), false);
  }

  virtual auto getBufferKind() const
      -> llvm::MemoryBuffer::BufferKind override {
    return MemoryBuffer_Malloc;
  }

  ~FileBuffer() {}

private:
};
} // namespace chocopy
