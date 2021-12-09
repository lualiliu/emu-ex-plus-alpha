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

#define LOGTAG "AppContext"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/VibrationManager.hh>
#include <imagine/input/Input.hh>
#include <imagine/fs/FS.hh>
#include <imagine/fs/AssetFS.hh>
#include <imagine/io/FileIO.hh>
#ifdef __ANDROID__
#include <imagine/fs/AAssetFS.hh>
#endif
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include <imagine/util/string.h>
#include <imagine/logger/logger.h>
#include <cstring>

namespace Base
{

void ApplicationContext::dispatchOnInit(ApplicationInitParams initParams)
{
	try
	{
		onInit(initParams);
	}
	catch(std::exception &err)
	{
		exitWithMessage(-1, err.what());
	}
}

Application &ApplicationContext::application() const
{
	return ApplicationContextImpl::application();
}

void ApplicationContext::runOnMainThread(MainThreadMessageDelegate del)
{
	application().runOnMainThread(del);
}

void ApplicationContext::flushMainThreadMessages()
{
	application().flushMainThreadMessages();
}

Window *ApplicationContext::makeWindow(WindowConfig config, WindowInitDelegate onInit)
{
	if(!Config::BASE_MULTI_WINDOW && windows().size())
	{
		bug_unreachable("no multi-window support");
	}
	auto winPtr = std::make_unique<Window>(*this, config, onInit);
	if(!*winPtr)
	{
		return nullptr;
	}
	auto ptr = winPtr.get();
	application().addWindow(std::move(winPtr));
	if(Window::shouldRunOnInitAfterAddingWindow && onInit)
	{
		try
		{
			onInit(*this, *ptr);
		}
		catch(std::exception &err)
		{
			exitWithMessage(-1, err.what());
		}
	}
	return ptr;
}

const WindowContainer &ApplicationContext::windows() const
{
	return application().windows();
}

Window &ApplicationContext::mainWindow()
{
	return application().mainWindow();
}

const ScreenContainer &ApplicationContext::screens() const
{
	return application().screens();
}

Screen &ApplicationContext::mainScreen()
{
	return application().mainScreen();
}

bool ApplicationContext::isRunning() const
{
	return application().isRunning();
}

bool ApplicationContext::isPaused() const
{
	return application().isPaused();
}

bool ApplicationContext::isExiting() const
{
	return application().isExiting();
}

void ApplicationContext::setOnInterProcessMessage(InterProcessMessageDelegate del)
{
	application().setOnInterProcessMessage(del);
}

bool ApplicationContext::addOnResume(ResumeDelegate del, int priority)
{
	return application().addOnResume(del, priority);
}

bool ApplicationContext::removeOnResume(ResumeDelegate del)
{
	return application().removeOnResume(del);
}

bool ApplicationContext::containsOnResume(ResumeDelegate del) const
{
	return application().containsOnResume(del);
}

void ApplicationContext::setOnFreeCaches(FreeCachesDelegate del)
{
	application().setOnFreeCaches(del);
}

bool ApplicationContext::addOnExit(ExitDelegate del, int priority)
{
	return application().addOnExit(del, priority);
}

bool ApplicationContext::removeOnExit(ExitDelegate del)
{
	return application().removeOnExit(del);
}

bool ApplicationContext::containsOnExit(ExitDelegate del) const
{
	return application().containsOnExit(del);
}

void ApplicationContext::dispatchOnInterProcessMessage(const char *filename)
{
	application().dispatchOnInterProcessMessage(*this, filename);
}

bool ApplicationContext::hasOnInterProcessMessage() const
{
	return application().hasOnInterProcessMessage();
}

void ApplicationContext::setOnScreenChange(ScreenChangeDelegate del)
{
	application().setOnScreenChange(del);
}

void ApplicationContext::dispatchOnResume(bool focused)
{
	application().dispatchOnResume(*this, focused);
}

void ApplicationContext::dispatchOnFreeCaches(bool running)
{
	application().dispatchOnFreeCaches(*this, running);
}

void ApplicationContext::dispatchOnExit(bool backgrounded)
{
	application().dispatchOnExit(*this, backgrounded);
}

FS::RootPathInfo ApplicationContext::nearestRootPath(std::string_view path) const
{
	if(path.empty())
		return {};
	auto location = rootFileLocations();
	const FS::PathLocation *nearestPtr{};
	size_t lastMatchOffset = 0;
	for(const auto &l : location)
	{
		if(!path.starts_with(l.path))
			continue;
		auto matchOffset = (size_t)(&path[l.root.length] - path.data());
		if(matchOffset > lastMatchOffset)
		{
			nearestPtr = &l;
			lastMatchOffset = matchOffset;
		}
	}
	if(!lastMatchOffset)
		return {};
	logMsg("found root location:%s with length:%d", nearestPtr->root.name.data(), (int)nearestPtr->root.length);
	return {nearestPtr->root.name, nearestPtr->root.length};
}

AssetIO ApplicationContext::openAsset(IG::CStringView name, IO::AccessHint hint, unsigned openFlags, const char *appName) const
{
	#ifdef __ANDROID__
	return {*this, name, hint, openFlags};
	#else
	return {FS::pathString(assetPath(appName), name), hint, openFlags};
	#endif
}

FS::AssetDirectoryIterator ApplicationContext::openAssetDirectory(IG::CStringView path, const char *appName)
{
	#ifdef __ANDROID__
	return {aAssetManager(), path};
	#else
	return {FS::pathString(assetPath(appName), path)};
	#endif
}

[[gnu::weak]] bool ApplicationContext::hasSystemPathPicker() const { return false; }

[[gnu::weak]] void ApplicationContext::showSystemPathPicker(SystemPathPickerDelegate, bool) {}

[[gnu::weak]] bool ApplicationContext::hasSystemDocumentPicker() const { return false; }

[[gnu::weak]] void ApplicationContext::showSystemDocumentPicker(SystemDocumentPickerDelegate) {}

[[gnu::weak]] FileIO ApplicationContext::openFileUri(IG::CStringView uri, IO::AccessHint access, unsigned openFlags) const
{
	return {uri, access, openFlags};
}

FileIO ApplicationContext::openFileUri(IG::CStringView uri, unsigned openFlags) const
{
	return openFileUri(uri, IO::AccessHint::NORMAL, openFlags);
}

[[gnu::weak]] FS::PathString ApplicationContext::fileUri(IG::CStringView uri, IG::CStringView name) const
{
	return FS::pathString(uri, name);
}

[[gnu::weak]] bool ApplicationContext::fileUriExists(IG::CStringView uri) const
{
	return FS::exists(uri);
}

[[gnu::weak]] std::string ApplicationContext::fileUriFormatLastWriteTimeLocal(IG::CStringView uri) const
{
	return FS::formatLastWriteTimeLocal(uri);
}

[[gnu::weak]] FS::FileString ApplicationContext::fileUriDisplayName(IG::CStringView uri) const
{
	return FS::basename(uri);
}

[[gnu::weak]] bool ApplicationContext::removeFileUri(IG::CStringView uri) const
{
	return FS::remove(uri);
}

Orientation ApplicationContext::validateOrientationMask(Orientation oMask) const
{
	if(!(oMask & VIEW_ROTATE_ALL))
	{
		// use default when none of the orientation bits are set
		oMask = defaultSystemOrientations();
	}
	return oMask;
}

const InputDeviceContainer &ApplicationContext::inputDevices() const
{
	return application().inputDevices();
}

void ApplicationContext::setHintKeyRepeat(bool on)
{
	application().setAllowKeyRepeatTimer(on);
}

Input::Event ApplicationContext::defaultInputEvent() const
{
	Input::Event e{};
	e.setMap(keyInputIsPresent() ? Input::Map::SYSTEM : Input::Map::POINTER);
	return e;
}

std::optional<bool> ApplicationContext::swappedConfirmKeysOption() const
{
	return application().swappedConfirmKeysOption();
}

bool ApplicationContext::swappedConfirmKeys() const
{
	return application().swappedConfirmKeys();
}

void ApplicationContext::setSwappedConfirmKeys(std::optional<bool> opt)
{
	application().setSwappedConfirmKeys(opt);
}

void ApplicationContext::setOnInputDeviceChange(InputDeviceChangeDelegate del)
{
	application().setOnInputDeviceChange(del);
}

void ApplicationContext::setOnInputDevicesEnumerated(InputDevicesEnumeratedDelegate del)
{
	application().setOnInputDevicesEnumerated(del);
}

[[gnu::weak]] void ApplicationContext::setSysUIStyle(uint32_t flags) {}

[[gnu::weak]] bool ApplicationContext::hasTranslucentSysUI() const { return false; }

[[gnu::weak]] bool ApplicationContext::hasHardwareNavButtons() const { return false; }

[[gnu::weak]] bool ApplicationContext::systemAnimatesWindowRotation() const { return Config::SYSTEM_ROTATES_WINDOWS; }

[[gnu::weak]] void ApplicationContext::setDeviceOrientationChangeSensor(bool) {}

[[gnu::weak]] void ApplicationContext::setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate) {}

[[gnu::weak]] void ApplicationContext::setSystemOrientation(Orientation) {}

[[gnu::weak]] Orientation ApplicationContext::defaultSystemOrientations() const { return VIEW_ROTATE_ALL; }

[[gnu::weak]] void ApplicationContext::setOnSystemOrientationChanged(SystemOrientationChangedDelegate) {}

[[gnu::weak]] bool ApplicationContext::usesPermission(Permission) const { return false; }

[[gnu::weak]] bool ApplicationContext::requestPermission(Permission) { return false; }

[[gnu::weak]] void ApplicationContext::addNotification(IG::CStringView onShow, IG::CStringView title, IG::CStringView message) {}

[[gnu::weak]] void ApplicationContext::addLauncherIcon(IG::CStringView name, IG::CStringView path) {}

[[gnu::weak]] bool VibrationManager::hasVibrator() const { return false; }

[[gnu::weak]] void VibrationManager::vibrate(IG::Milliseconds) {}

[[gnu::weak]] NativeDisplayConnection ApplicationContext::nativeDisplayConnection() const { return {}; }

[[gnu::weak]] bool ApplicationContext::packageIsInstalled(IG::CStringView name) const { return false; }

OnExit::OnExit(ResumeDelegate del, ApplicationContext ctx, int priority): del{del}, ctx{ctx}
{
	ctx.addOnExit(del, priority);
}

OnExit::OnExit(OnExit &&o)
{
	*this = std::move(o);
}

OnExit &OnExit::operator=(OnExit &&o)
{
	reset();
	del = std::exchange(o.del, {});
	ctx = o.ctx;
	return *this;
}

OnExit::~OnExit()
{
	reset();
}

void OnExit::reset()
{
	if(!del)
		return;
	ctx.removeOnExit(std::exchange(del, {}));
}

ApplicationContext OnExit::appContext() const
{
	return ctx;
}

}

namespace FileUtils
{

ssize_t writeToUri(Base::ApplicationContext ctx, IG::CStringView uri, void *data, size_t bytes)
{
	auto f = ctx.openFileUri(uri, IO::OPEN_CREATE | IO::OPEN_TEST);
	return f.write(data, bytes);
}

ssize_t readFromUri(Base::ApplicationContext ctx, IG::CStringView uri, void *data, size_t size)
{
	auto f = ctx.openFileUri(uri, IO::AccessHint::SEQUENTIAL, IO::OPEN_TEST);
	return f.read(data, size);
}

IG::ByteBuffer bufferFromUri(Base::ApplicationContext ctx, IG::CStringView uri, unsigned openFlags, size_t sizeLimit)
{
	auto file = ctx.openFileUri(uri, IO::AccessHint::ALL, openFlags);
	if(!file)
		return {};
	if(file.size() > sizeLimit)
	{
		if(openFlags & IO::OPEN_TEST)
			return {};
		else
			throw std::runtime_error(fmt::format("{} exceeds {} byte limit", uri.data(), sizeLimit));
	}
	return file.buffer(IODefs::BufferMode::RELEASE);
}

FILE *fopenUri(Base::ApplicationContext ctx, IG::CStringView path, IG::CStringView mode)
{
	if(FS::isUri(path))
	{
		int openFlags = IG::stringContains(mode, 'w') ? IO::OPEN_CREATE : 0;
		return GenericIO{ctx.openFileUri(path, openFlags | IO::OPEN_TEST)}.moveToFileStream(mode);
	}
	else
	{
		return ::fopen(path, mode);
	}
}

}
