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

#define LOGTAG "PosixFileIO"
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/fd-utils.h>
#include <imagine/util/variant.hh>
#include "IOUtils.hh"
#include <sys/mman.h>
#include <errno.h>
#include <cstring>

namespace IG
{

#if defined __linux__
static constexpr bool hasMmapPopulateFlag = true;
#else
static constexpr bool hasMmapPopulateFlag = false;
static constexpr int MAP_POPULATE = 0;
#endif

template class IOUtils<PosixFileIO>;

static void applyAccessHint(PosixFileIO &io, IO::AccessHint access, bool isMapped)
{
	switch(access)
	{
		case IO::AccessHint::NORMAL:
		case IO::AccessHint::UNMAPPED: return;
		case IO::AccessHint::SEQUENTIAL: return io.advise(0, 0, IO::Advice::SEQUENTIAL);
		case IO::AccessHint::RANDOM: return io.advise(0, 0, IO::Advice::RANDOM);
		case IO::AccessHint::ALL:
			if(!isMapped || (isMapped && !hasMmapPopulateFlag))
				return io.advise(0, 0, IO::Advice::WILLNEED);
	}
}

PosixFileIO::PosixFileIO(UniqueFileDescriptor fd_, IO::AccessHint access, IO::OpenFlags openFlags):
	ioImpl{std::in_place_type<PosixIO>, std::move(fd_)}
{
	tryMmap(std::get_if<PosixIO>(&ioImpl)->fd(), access, openFlags);
}

PosixFileIO::PosixFileIO(UniqueFileDescriptor fd, IO::OpenFlags openFlags):
	PosixFileIO{std::move(fd), IO::AccessHint::NORMAL, openFlags} {}

PosixFileIO::PosixFileIO(IG::CStringView path, IO::AccessHint access, IO::OpenFlags openFlags):
	ioImpl{std::in_place_type<PosixIO>, path, openFlags}
{
	tryMmap(std::get_if<PosixIO>(&ioImpl)->fd(), access, openFlags);
}

PosixFileIO::PosixFileIO(IG::CStringView path, IO::OpenFlags openFlags):
	PosixFileIO{path, IO::AccessHint::NORMAL, openFlags} {}

void PosixFileIO::tryMmap(int fd, IO::AccessHint access, IO::OpenFlags openFlags)
{
	// try to open as memory map only if read-only
	if(openFlags & IO::OPEN_WRITE || access == IO::AccessHint::UNMAPPED || fd == -1)
		return;
	MapIO mappedFile = makePosixMapIO(access, fd);
	if(mappedFile)
	{
		ioImpl = std::move(mappedFile);
		applyAccessHint(*this, access, true);
	}
	else
	{
		applyAccessHint(*this, access, false);
	}
}

FileIO FileIO::create(IG::CStringView path, IO::OpenFlags mode)
{
	mode |= IO::OPEN_CREATE;
	return {path, IO::AccessHint::NORMAL, mode};
}

static IO& getIO(std::variant<PosixIO, MapIO> &ioImpl)
{
	return visit([](auto &io) -> IO& { return io; }, ioImpl);
}

PosixFileIO::operator IO*() { return &getIO(ioImpl); }

PosixFileIO::operator IO&() { return getIO(ioImpl); }

PosixFileIO::operator GenericIO()
{
	return visit([](auto &&io) { return GenericIO{std::move(io)}; }, ioImpl);
}

static IG::ByteBuffer byteBufferFromMmap(void *data, off_t size)
{
	return
	{
		{(uint8_t*)data, (size_t)size},
		[](const uint8_t *ptr, size_t size)
		{
			logMsg("unmapping:%p (%zu bytes)", ptr, size);
			if(munmap((void*)ptr, size) == -1)
			{
				if(Config::DEBUG_BUILD)
					logErr("munmap(%p, %zu) error:%s", ptr, size, strerror(errno));
			}
		}
	};
}

MapIO PosixFileIO::makePosixMapIO(IO::AccessHint access, int fd)
{
	off_t size = fd_size(fd);
	if(!size) [[unlikely]]
		return {};
	int flags = MAP_SHARED;
	if(access == IO::AccessHint::ALL)
		flags |= MAP_POPULATE;
	void *data = mmap(nullptr, size, PROT_READ, flags, fd, 0);
	if(data == MAP_FAILED)
	{
		logErr("mmap fd:%d (%zu bytes) failed", fd, (size_t)size);
		return {};
	}
	logMsg("mapped fd:%d to %p (%zu bytes)", fd, data, (size_t)size);
	return {byteBufferFromMmap(data, size)};
}

ssize_t PosixFileIO::read(void *buff, size_t bytes)
{
	return visit([&](auto &io){ return io.read(buff, bytes); }, ioImpl);
}

ssize_t PosixFileIO::readAtPos(void *buff, size_t bytes, off_t offset)
{
	return visit([&](auto &io){ return io.readAtPos(buff, bytes, offset); }, ioImpl);
}

std::span<uint8_t> PosixFileIO::map()
{
	return visit([&](auto &io){ return io.map(); }, ioImpl);
}

ssize_t PosixFileIO::write(const void *buff, size_t bytes)
{
	return visit([&](auto &io){ return io.write(buff, bytes); }, ioImpl);
}

bool PosixFileIO::truncate(off_t offset)
{
	return visit([&](auto &io){ return io.truncate(offset); }, ioImpl);
}

off_t PosixFileIO::seek(off_t offset, IO::SeekMode mode)
{
	return visit([&](auto &io){ return io.seek(offset, mode); }, ioImpl);
}

void PosixFileIO::sync()
{
	visit([&](auto &io){ io.sync(); }, ioImpl);
}

size_t PosixFileIO::size()
{
	return visit([&](auto &io){ return io.size(); }, ioImpl);
}

bool PosixFileIO::eof()
{
	return visit([&](auto &io){ return io.eof(); }, ioImpl);
}

void PosixFileIO::advise(off_t offset, size_t bytes, IO::Advice advice)
{
	visit([&](auto &io){ io.advise(offset, bytes, advice); }, ioImpl);
}

PosixFileIO::operator bool() const
{
	return visit([&](auto &io){ return (bool)io; }, ioImpl);
}

IG::ByteBuffer PosixFileIO::releaseBuffer()
{
	auto mapIoPtr = std::get_if<MapIO>(&ioImpl);
	if(!mapIoPtr)
		return {};
	return mapIoPtr->releaseBuffer();
}

UniqueFileDescriptor PosixFileIO::releaseFd()
{
	if(auto ioPtr = std::get_if<PosixIO>(&ioImpl);
		ioPtr)
	{
		return ioPtr->releaseFd();
	}
	logWarn("trying to release fd of mapped IO");
	return {};
}

}
