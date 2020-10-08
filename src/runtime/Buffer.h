#pragma once

#include <fstream>
#include <istream>
#include <ostream>

#include <lz4.h>

#include "IG_Config.h"

namespace IG {
namespace IO {
inline void skip_buffer(std::istream& is)
{
	size_t in_size = 0, out_size = 0;
	is.read((char*)&in_size, sizeof(uint32));
	is.read((char*)&out_size, sizeof(uint32));
	is.seekg(out_size, std::ios::cur);
}

template <typename Array>
inline void decompress(const std::vector<char>& in, Array& out)
{
	LZ4_decompress_safe(in.data(), (char*)out.data(), in.size(), out.size() * sizeof(out[0]));
}

template <typename Array>
inline void read_buffer(std::istream& is, Array& array)
{
	size_t in_size = 0, out_size = 0;
	is.read((char*)&in_size, sizeof(uint32));
	is.read((char*)&out_size, sizeof(uint32));
	std::vector<char> in(out_size);
	is.read(in.data(), in.size());
	array = std::move(Array(in_size / sizeof(array[0])));
	decompress(in, array);
}

template <typename Array>
inline void read_buffer(const std::string& file_name, Array& array)
{
	std::ifstream is(file_name, std::ios::binary);
	read_buffer(is, array);
}

template <typename Array>
inline void compress(const Array& in, std::vector<char>& out)
{
	size_t in_size = sizeof(in[0]) * in.size();
	out.resize(LZ4_compressBound(in_size));
	out.resize(LZ4_compress_default((const char*)in.data(), out.data(), in_size, out.size()));
}

template <typename Array>
inline void write_buffer(std::ostream& os, const Array& array)
{
	std::vector<char> out;
	compress(array, out);
	size_t in_size	= sizeof(array[0]) * array.size();
	size_t out_size = out.size();
	os.write((char*)&in_size, sizeof(uint32));
	os.write((char*)&out_size, sizeof(uint32));
	os.write(out.data(), out.size());
}

template <typename Array>
inline void write_buffer(const std::string& file_name, const Array& array)
{
	std::ofstream of(file_name, std::ios::binary);
	write_buffer(of, array);
}

} // namespace IO
} // namespace IG