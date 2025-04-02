module;
#include "llvm/Support/MemoryBuffer.h"

export module FileBuffer;

export namespace chocopy {
class FileBuffer : public llvm::MemoryBuffer {
public:
	using Base = llvm::MemoryBuffer;
	FileBuffer(std::string Str) {
		Base::init(Str.data(), Str.data() + Str.size(), false);
	}

	FileBuffer(FileBuffer&& Other) : Buf(std::move(Other.Buf)) {
		Base::init(Buf.data(), Buf.data() + Buf.size(), false);
	}
	FileBuffer& operator=(FileBuffer&& Other) {
		Buf = std::move(Other.Buf);
		Base::init(Buf.data(), Buf.data() + Buf.size(), false);
		return *this;
	}

	virtual auto getBufferKind() const -> llvm::MemoryBuffer::BufferKind override {
		return MemoryBuffer_Malloc;
	}

	~FileBuffer() {}

private:
	std::string Buf;
};
} // namespace chocopy
