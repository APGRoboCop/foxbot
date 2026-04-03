//
// FoXBot - AI Bot for Halflife's Team Fortress Classic
//
// (http://foxbot.net)
//
// bot_compress.h
//
// Zlib compression helpers for saving/loading binary data files.
// Uses miniz (public domain zlib replacement) for compression.
//
// Compressed files use a small header to identify them:
//   4 bytes: magic "FZC\0" (FoXBot Zlib Compressed)
//   4 bytes: uncompressed data size (uint32)
//   4 bytes: compressed data size (uint32)
//   N bytes: zlib-compressed data
//

#ifndef BOT_COMPRESS_H
#define BOT_COMPRESS_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include "miniz.h"

// Magic bytes to identify FoXBot compressed files
constexpr char FZC_MAGIC[4] = { 'F', 'Z', 'C', '\0' };
constexpr int FZC_HEADER_SIZE = 12; // magic(4) + uncompressed_size(4) + compressed_size(4)

// Write data to a file with zlib compression.
// Returns true on success.
inline bool FoxCompressedWrite(std::FILE* fp, const unsigned char* data, const unsigned int dataSize)
{
	if (fp == nullptr || data == nullptr || dataSize == 0)
		return false;

	// compute upper bound for compressed data
	const mz_ulong compBound = mz_compressBound(static_cast<mz_ulong>(dataSize));
	auto* compBuf = static_cast<unsigned char*>(std::malloc(compBound));
	if (compBuf == nullptr)
		return false;

	mz_ulong compSize = compBound;
	const int status = mz_compress2(compBuf, &compSize, data, static_cast<mz_ulong>(dataSize), MZ_BEST_COMPRESSION);
	if (status != MZ_OK)
	{
		std::free(compBuf);
		return false;
	}

	// write header
	std::fwrite(FZC_MAGIC, 1, 4, fp);

	const unsigned int uncompSize = dataSize;
	std::fwrite(&uncompSize, sizeof(unsigned int), 1, fp);

	const auto compSizeU = static_cast<unsigned int>(compSize);
	std::fwrite(&compSizeU, sizeof(unsigned int), 1, fp);

	// write compressed data
	const size_t written = std::fwrite(compBuf, 1, static_cast<size_t>(compSize), fp);
	std::free(compBuf);

	return written == static_cast<size_t>(compSize);
}

// Read a compressed file and decompress it. Allocates the output buffer with malloc().
// The caller must free() the returned buffer.
// On failure returns nullptr and sets outSize to 0.
// If the file does not have the FZC magic header, it reads the raw file instead
// (backward compatibility with uncompressed files).
inline unsigned char* FoxCompressedRead(std::FILE* fp, unsigned int* outSize)
{
	if (fp == nullptr || outSize == nullptr)
		return nullptr;

	*outSize = 0;

	// read the first 4 bytes to check for magic
	char magic[4] = {};
	if (std::fread(magic, 1, 4, fp) != 4)
		return nullptr;

	if (std::memcmp(magic, FZC_MAGIC, 4) == 0)
	{
		// compressed file
		unsigned int uncompSize = 0, compSize = 0;
		if (std::fread(&uncompSize, sizeof(unsigned int), 1, fp) != 1)
			return nullptr;
		if (std::fread(&compSize, sizeof(unsigned int), 1, fp) != 1)
			return nullptr;

		if (uncompSize == 0 || compSize == 0 || uncompSize > 4 * 1024 * 1024)
			return nullptr; // sanity check: 4 MB max

		auto* compBuf = static_cast<unsigned char*>(std::malloc(compSize));
		if (compBuf == nullptr)
			return nullptr;

		if (std::fread(compBuf, 1, compSize, fp) != compSize)
		{
			std::free(compBuf);
			return nullptr;
		}

		auto* outBuf = static_cast<unsigned char*>(std::malloc(uncompSize));
		if (outBuf == nullptr)
		{
			std::free(compBuf);
			return nullptr;
		}

		mz_ulong destLen = static_cast<mz_ulong>(uncompSize);
		const int status = mz_uncompress(outBuf, &destLen, compBuf, static_cast<mz_ulong>(compSize));
		std::free(compBuf);

		if (status != MZ_OK || destLen != static_cast<mz_ulong>(uncompSize))
		{
			std::free(outBuf);
			return nullptr;
		}

		*outSize = uncompSize;
		return outBuf;
	}
	else
	{
		// not compressed - read the whole file as raw data (backward compatibility)
		std::fseek(fp, 0, SEEK_END);
		const long fileSize = std::ftell(fp);
		std::fseek(fp, 0, SEEK_SET);

		if (fileSize <= 0 || fileSize > 4 * 1024 * 1024)
			return nullptr;

		auto* outBuf = static_cast<unsigned char*>(std::malloc(static_cast<size_t>(fileSize)));
		if (outBuf == nullptr)
			return nullptr;

		if (std::fread(outBuf, 1, static_cast<size_t>(fileSize), fp) != static_cast<size_t>(fileSize))
		{
			std::free(outBuf);
			return nullptr;
		}

		*outSize = static_cast<unsigned int>(fileSize);
		return outBuf;
	}
}

// Check if a file starts with the FZC compressed magic.
// Seeks back to the original position afterwards.
inline bool FoxIsCompressedFile(std::FILE* fp)
{
	if (fp == nullptr)
		return false;

	const long pos = std::ftell(fp);
	char magic[4] = {};
	const bool isCompressed = (std::fread(magic, 1, 4, fp) == 4 && std::memcmp(magic, FZC_MAGIC, 4) == 0);
	std::fseek(fp, pos, SEEK_SET);
	return isCompressed;
}

#endif // BOT_COMPRESS_H
