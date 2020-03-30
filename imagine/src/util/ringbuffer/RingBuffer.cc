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

#define LOGTAG "RingBuffer"
#include <imagine/util/ringbuffer/RingBuffer.hh>
#include <imagine/util/ringbuffer/mirroredBuffer.hh>
#include <imagine/util/utility.h>
#include <imagine/logger/logger.h>
#include <cstring>

namespace IG
{

using SizeType = RingBuffer::SizeType;

RingBuffer::RingBuffer(SizeType size)
{
	init(adjustMirroredBufferAllocSize(size));
}

RingBuffer::RingBuffer(RingBuffer &&o)
{
	*this = std::move(o);
}

RingBuffer &RingBuffer::operator=(RingBuffer &&o)
{
	deinit();
	buff = std::exchange(o.buff, {});
	start = std::exchange(o.start, {});
	end = std::exchange(o.end, {});
	buffSize = std::exchange(o.buffSize, 0);
	written = o.written.exchange(0, std::memory_order_relaxed);
	return *this;
}

RingBuffer::~RingBuffer()
{
	deinit();
}

void RingBuffer::init(SizeType size)
{
	assert(!buff);
	logMsg("allocating with capacity:%u", size);
	buff = (char*)allocMirroredBuffer(size);
	if(!buff)
	{
		return;
	}
	buffSize = size;
	clear();
}

void RingBuffer::deinit()
{
	freeMirroredBuffer(buff, buffSize);
	buff = nullptr;
	buffSize = 0;
	written.store(0, std::memory_order_relaxed);
}

void RingBuffer::clear()
{
	start = end = buff;
	written.store(0, std::memory_order_relaxed);
}

SizeType RingBuffer::freeSpace() const
{
	return capacity() - size();
}

SizeType RingBuffer::size() const
{
	return written;
}

SizeType RingBuffer::capacity() const
{
	return buffSize;
}

void RingBuffer::setMinCapacity(SizeType capacity)
{
	SizeType realCapacity = adjustMirroredBufferAllocSize(capacity);
	if(realCapacity == buffSize)
		return;
	SizeType oldSize = size();
	SizeType oldCapacity = buffSize;
	char *oldBuff{};
	if(oldSize)
	{
		logMsg("copying %u bytes due to capacity change", oldSize);
		oldBuff = buff;
		buff = nullptr;
	}
	deinit();
	init(realCapacity);
	if(oldBuff)
	{
		// copy old contents
		write(oldBuff, oldSize);
		freeMirroredBuffer(oldBuff, oldCapacity);
	}
}

SizeType RingBuffer::write(const void *buff, SizeType size)
{
	auto space = freeSpace();
	if(size > space)
		size = space;
	return writeUnchecked(buff, size);
}

SizeType RingBuffer::writeUnchecked(const void *buff, SizeType size)
{
	assert(size <= freeSpace());
	memcpy(end, buff, size);
	commitWrite(size);
	//logMsg("wrote %d bytes", (int)size);
	return size;
}

char *RingBuffer::writeAddr() const
{
	return end;
}

void RingBuffer::commitWrite(SizeType size)
{
	assert(size <= freeSpace());
	end = advanceAddr(end, size);
	written += size;
}

SizeType RingBuffer::read(void *buff, SizeType size_)
{
	auto writtenSize = size();
	if(size_ > writtenSize)
		size_ = writtenSize;
	memcpy(buff, start, size_);
	commitRead(size_);
	//logMsg("read %d bytes", (int)size);
	return size_;
}

char *RingBuffer::readAddr() const
{
	return start;
}

void RingBuffer::commitRead(SizeType size_)
{
	assert(size_ <= size());
	start = advanceAddr(start, size_);
	written -= size_;
}

char *RingBuffer::advanceAddr(char *ptr, SizeType size) const
{
	return wrapPtr(ptr + size);
}

char *RingBuffer::wrapPtr(char *ptr) const
{
	if(ptr >= buff + buffSize)
		ptr -= buffSize;
	return ptr;
}

RingBuffer::operator bool() const
{
	return buff;
}

}