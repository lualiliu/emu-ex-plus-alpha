#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/io/PosixFileIO.hh>
#ifdef __ANDROID__
#include <imagine/io/AAssetIO.hh>
#endif
#include <imagine/util/string/CStringView.hh>
#include <span>

namespace IG
{

class FileIO: public PosixFileIO
{
public:
	using PosixFileIO::PosixFileIO;

	[[nodiscard]]
	static FileIO create(IG::CStringView path, IO::OpenFlags oFlags = {});
};

#ifdef __ANDROID__
using AssetIOImpl = AAssetIO;
#else
using AssetIOImpl = FileIO;
#endif

class AssetIO: public AssetIOImpl
{
public:
	using AssetIOImpl::AssetIOImpl;
};

class ApplicationContext;

}

namespace IG::FS
{
class PathString;
}

namespace IG::FileUtils
{

static constexpr size_t defaultBufferReadSizeLimit = 0x2000000; // 32 Megabytes

ssize_t writeToPath(CStringView path, std::span<const unsigned char> src);
ssize_t writeToPath(CStringView path, IO &io);
ssize_t writeToUri(ApplicationContext ctx, CStringView uri, std::span<const unsigned char> src);
ssize_t readFromPath(IG::CStringView path, std::span<unsigned char> dest, IO::AccessHint accessHint = IO::AccessHint::ALL);
ssize_t readFromUri(ApplicationContext, CStringView uri, std::span<unsigned char> dest,
	IO::AccessHint accessHint = IO::AccessHint::ALL);
std::pair<ssize_t, FS::PathString> readFromUriWithArchiveScan(ApplicationContext, IG::CStringView uri,
	std::span<unsigned char> dest, bool(*nameMatchFunc)(std::string_view), IO::AccessHint accessHint = IO::AccessHint::ALL);
IG::ByteBuffer bufferFromPath(CStringView path, IO::OpenFlags oFlags = {}, size_t sizeLimit = defaultBufferReadSizeLimit);
IG::ByteBuffer bufferFromUri(ApplicationContext, CStringView uri, IO::OpenFlags oFlags = {}, size_t sizeLimit = defaultBufferReadSizeLimit);
FILE *fopenUri(ApplicationContext, CStringView path, CStringView mode);

}
