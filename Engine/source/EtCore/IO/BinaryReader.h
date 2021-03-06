#pragma once


namespace et {
namespace core {


//-------------------------
// BinaryReader
//
// Utility class for parsing binary content
//
class BinaryReader final
{
	// definitions
	//-------------
public:
	static size_t const s_InvalidBufferPos;

	// construct destruct
	//--------------------
	BinaryReader() = default;
	~BinaryReader() = default;

	BinaryReader(BinaryReader const& rhs) = delete;
	BinaryReader& operator=(BinaryReader const& rhs) = delete;

	// functionality
	//---------------
	void Open(std::vector<uint8> const& binaryContent, size_t const start = 0u, size_t const count = 0u);
	void Close();

	void SetBufferPosition(size_t const pos);
	void MoveBufferPosition(size_t const move);

	// accessors
	//-----------
	size_t GetBufferPosition() const;
	size_t GetBufferSize() const { return m_BufferSize; }
	bool Exists() const { return (m_BinData != nullptr); }

	template<typename TDataType>
	TDataType Read();
	template<typename TDataType>
	TDataType const& ReadRef();

	void ReadData(uint8* const data, size_t const size);
	uint8 const* GetCurrentDataPointer() const;

	template<typename TStringLengthType>
	std::string ReadString();
	std::string ReadString(size_t const size);
	std::string ReadNullString();

	template <uint8 n, class T>
	math::vector<n, T> ReadVector();
	template <uint8 m, uint8 n, class T>
	math::matrix<m, n, T> ReadMatrix();


	// Data
	///////

private:

	size_t m_BufferPosition = s_InvalidBufferPos;
	size_t m_BufferStart = 0u;
	size_t m_BufferSize = s_InvalidBufferPos;

	std::vector<uint8> const* m_BinData = nullptr;
};


} // namespace core
} // namespace et


#include "BinaryReader.inl"
