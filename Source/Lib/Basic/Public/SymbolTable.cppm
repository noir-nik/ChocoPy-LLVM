export module Basic:SymbolTable;
import LLVM;
import :TokenKinds;

export namespace chocopy {
class Declaration;
using llvm::StringRef;

class SymbolInfo {
	friend class SymbolTable;

public:
	tok::TokenKind getKind() const { return TokenID; }

	StringRef getName() const { return Entry->getKey(); }

	void* getFETokenInfo() const { return FETokenInfo; }
	void  setFETokenInfo(void* T) { FETokenInfo = T; }

private:
	tok::TokenKind                     TokenID     = tok::TokenKind::identifier;
	llvm::StringMapEntry<SymbolInfo*>* Entry       = nullptr;
	void*                              FETokenInfo = nullptr;
};

class SymbolTable {
public:
	SymbolInfo& get(StringRef Name) {
		HashTableTy::iterator              Item  = HashTable.try_emplace(Name, nullptr).first;
		llvm::StringMapEntry<SymbolInfo*>& Entry = *Item;
		if (Entry.second)
			return *Entry.second;
		llvm::BumpPtrAllocator& Allocator = getAllocator();

		void*       Mem   = Allocator.Allocate<SymbolInfo>();
		SymbolInfo* SInfo = new (Mem) SymbolInfo();

		SInfo->Entry = &Entry;
		Entry.second = SInfo;
		return *SInfo;
	}

	SymbolInfo& get(StringRef Name, tok::TokenKind Kind) {
		SymbolInfo& SI = get(Name);
		SI.TokenID     = Kind;
		return SI;
	}

private:
	llvm::BumpPtrAllocator& getAllocator() { return HashTable.getAllocator(); }

private:
	using HashTableTy = llvm::StringMap<SymbolInfo*, llvm::BumpPtrAllocator>;

	HashTableTy HashTable;
};
} // namespace chocopy
