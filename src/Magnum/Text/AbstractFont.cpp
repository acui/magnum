/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "AbstractFont.h"

#include <string> /** @todo remove once file callbacks are <string>-free */
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/EnumSet.hpp>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once file callbacks are <string>-free */
#include <Corrade/Containers/Triple.h>
#include <Corrade/PluginManager/Manager.hpp>
#include <Corrade/Utility/Path.h>
#include <Corrade/Utility/Unicode.h>

#include "Magnum/FileCallback.h"
#include "Magnum/Math/Functions.h"
#include "Magnum/Math/Range.h"
#include "Magnum/Text/AbstractGlyphCache.h"

#ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
#include "Magnum/Text/configure.h"
#endif

namespace Corrade { namespace PluginManager {

template class MAGNUM_TEXT_EXPORT Manager<Magnum::Text::AbstractFont>;

}}

namespace Magnum { namespace Text {

using namespace Containers::Literals;

Containers::StringView AbstractFont::pluginInterface() {
    return MAGNUM_TEXT_ABSTRACTFONT_PLUGIN_INTERFACE ""_s;
}

#ifndef CORRADE_PLUGINMANAGER_NO_DYNAMIC_PLUGIN_SUPPORT
Containers::Array<Containers::String> AbstractFont::pluginSearchPaths() {
    const Containers::Optional<Containers::String> libraryLocation = Utility::Path::libraryLocation(&pluginInterface);
    return PluginManager::implicitPluginSearchPaths(
        #ifndef MAGNUM_BUILD_STATIC
        libraryLocation ? *libraryLocation : Containers::String{},
        #else
        {},
        #endif
        #ifdef CORRADE_IS_DEBUG_BUILD
        MAGNUM_PLUGINS_FONT_DEBUG_DIR,
        #else
        MAGNUM_PLUGINS_FONT_DIR,
        #endif
        #ifdef CORRADE_IS_DEBUG_BUILD
        "magnum-d/"
        #else
        "magnum/"
        #endif
        "fonts"_s);
}
#endif

AbstractFont::AbstractFont() = default;

AbstractFont::AbstractFont(PluginManager::AbstractManager& manager, const Containers::StringView& plugin): AbstractPlugin{manager, plugin} {}

void AbstractFont::setFileCallback(Containers::Optional<Containers::ArrayView<const char>>(*callback)(const std::string&, InputFileCallbackPolicy, void*), void* const userData) {
    CORRADE_ASSERT(!isOpened(), "Text::AbstractFont::setFileCallback(): can't be set while a font is opened", );
    CORRADE_ASSERT(features() & (FontFeature::FileCallback|FontFeature::OpenData), "Text::AbstractFont::setFileCallback(): font plugin supports neither loading from data nor via callbacks, callbacks can't be used", );

    _fileCallback = callback;
    _fileCallbackUserData = userData;
    doSetFileCallback(callback, userData);
}

void AbstractFont::doSetFileCallback(Containers::Optional<Containers::ArrayView<const char>>(*)(const std::string&, InputFileCallbackPolicy, void*), void*) {}

bool AbstractFont::openData(Containers::ArrayView<const void> data, const Float size) {
    CORRADE_ASSERT(features() & FontFeature::OpenData,
        "Text::AbstractFont::openData(): feature not supported", false);

    /* We accept empty data here (instead of checking for them and failing so
       the check doesn't be done on the plugin side) because for some file
       formats it could be valid (MagnumFont in particular). */
    close();
    const Properties properties = doOpenData(Containers::arrayCast<const char>(data), size);

    /* If opening succeeded, save the returned values. If not, the values were
       set to their default values by close() already. */
    if(isOpened()) {
        _size = properties.size;
        _ascent = properties.ascent;
        _descent = properties.descent;
        _lineHeight = properties.lineHeight;
        _glyphCount = properties.glyphCount;
        return true;
    }

    return false;
}

auto AbstractFont::doOpenData(Containers::ArrayView<const char>, Float) -> Properties {
    CORRADE_ASSERT_UNREACHABLE("Text::AbstractFont::openData(): feature advertised but not implemented", {});
}

bool AbstractFont::openFile(const Containers::StringView filename, const Float size) {
    close();
    Properties properties;

    /* If file loading callbacks are not set or the font implementation
       supports handling them directly, call into the implementation */
    if(!_fileCallback || (doFeatures() & FontFeature::FileCallback)) {
        properties = doOpenFile(filename, size);

    /* Otherwise, if loading from data is supported, use the callback and pass
       the data through to openData(). Mark the file as ready to be closed once
       opening is finished. */
    } else if(doFeatures() & FontFeature::OpenData) {
        /* This needs to be duplicated here and in the doOpenFile()
           implementation in order to support both following cases:
            - plugins that don't support FileCallback but have their own
              doOpenFile() implementation (callback needs to be used here,
              because the base doOpenFile() implementation might never get
              called)
            - plugins that support FileCallback but want to delegate the actual
              file loading to the default implementation (callback used in the
              base doOpenFile() implementation, because this branch is never
              taken in that case) */
        const Containers::Optional<Containers::ArrayView<const char>> data = _fileCallback(filename, InputFileCallbackPolicy::LoadTemporary, _fileCallbackUserData);
        if(!data) {
            Error() << "Text::AbstractFont::openFile(): cannot open file" << filename;
            return isOpened();
        }

        properties = doOpenData(*data, size);
        _fileCallback(filename, InputFileCallbackPolicy::Close, _fileCallbackUserData);

    /* Shouldn't get here, the assert is fired already in setFileCallback() */
    } else CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */

    /* If opening succeeded, save the returned values. If not, the values were
       set to their default values by close() already. */
    if(isOpened()) {
        _size = properties.size;
        _ascent = properties.ascent;
        _descent = properties.descent;
        _lineHeight = properties.lineHeight;
        _glyphCount = properties.glyphCount;
        return true;
    }

    return false;
}

auto AbstractFont::doOpenFile(const Containers::StringView filename, const Float size) -> Properties {
    CORRADE_ASSERT(features() & FontFeature::OpenData, "Text::AbstractFont::openFile(): not implemented", {});

    Properties properties;

    /* If callbacks are set, use them. This is the same implementation as in
       openFile(), see the comment there for details. */
    if(_fileCallback) {
        const Containers::Optional<Containers::ArrayView<const char>> data = _fileCallback(filename, InputFileCallbackPolicy::LoadTemporary, _fileCallbackUserData);
        if(!data) {
            Error() << "Text::AbstractFont::openFile(): cannot open file" << filename;
            return {};
        }

        properties = doOpenData(*data, size);
        _fileCallback(filename, InputFileCallbackPolicy::Close, _fileCallbackUserData);

    /* Otherwise open the file directly */
    } else {
        const Containers::Optional<Containers::Array<char>> data = Utility::Path::read(filename);
        if(!data) {
            Error() << "Text::AbstractFont::openFile(): cannot open file" << filename;
            return {};
        }

        properties = doOpenData(*data, size);
    }

    return properties;
}

void AbstractFont::close() {
    if(!isOpened()) return;

    doClose();
    CORRADE_INTERNAL_ASSERT(!isOpened());

    /* Clear the saved values to avoid accidental use of stale (state even
       though their public access is guarded with isOpened()) */
    _size = {};
    _lineHeight = {};
    _descent = {};
    _lineHeight = {};
}

Float AbstractFont::size() const {
    CORRADE_ASSERT(isOpened(), "Text::AbstractFont::size(): no font opened", {});
    return _size;
}

Float AbstractFont::ascent() const {
    CORRADE_ASSERT(isOpened(), "Text::AbstractFont::ascent(): no font opened", {});
    return _ascent;
}

Float AbstractFont::descent() const {
    CORRADE_ASSERT(isOpened(), "Text::AbstractFont::descent(): no font opened", {});
    return _descent;
}

Float AbstractFont::lineHeight() const {
    CORRADE_ASSERT(isOpened(), "Text::AbstractFont::lineHeight(): no font opened", {});
    return _lineHeight;
}

UnsignedInt AbstractFont::glyphCount() const {
    CORRADE_ASSERT(isOpened(), "Text::AbstractFont::glyphCount(): no font opened", 0);
    return _glyphCount;
}

UnsignedInt AbstractFont::glyphId(const char32_t character) {
    CORRADE_ASSERT(isOpened(), "Text::AbstractFont::glyphId(): no font opened", 0);

    return doGlyphId(character);
}

Vector2 AbstractFont::glyphSize(const UnsignedInt glyph) {
    CORRADE_ASSERT(isOpened(), "Text::AbstractFont::glyphSize(): no font opened", {});
    CORRADE_ASSERT(glyph < _glyphCount, "Text::AbstractFont::glyphSize(): index" << glyph << "out of range for" << _glyphCount << "glyphs", {});

    return doGlyphSize(glyph);
}

Vector2 AbstractFont::glyphAdvance(const UnsignedInt glyph) {
    CORRADE_ASSERT(isOpened(), "Text::AbstractFont::glyphAdvance(): no font opened", {});
    CORRADE_ASSERT(glyph < _glyphCount, "Text::AbstractFont::glyphAdvance(): index" << glyph << "out of range for" << _glyphCount << "glyphs", {});

    return doGlyphAdvance(glyph);
}

void AbstractFont::fillGlyphCache(AbstractGlyphCache& cache, const Containers::StringView characters) {
    CORRADE_ASSERT(isOpened(),
        "Text::AbstractFont::fillGlyphCache(): no font opened", );
    CORRADE_ASSERT(!(features() & FontFeature::PreparedGlyphCache),
        "Text::AbstractFont::fillGlyphCache(): feature not supported", );

    const Containers::Optional<Containers::Array<char32_t>> utf32 = Utility::Unicode::utf32(characters);
    CORRADE_ASSERT(utf32,
        "Text::AbstractFont::fillGlyphCache(): not a valid UTF-8 string:" << characters, );

    doFillGlyphCache(cache, *utf32);
}

void AbstractFont::doFillGlyphCache(AbstractGlyphCache&, Containers::ArrayView<const char32_t>) {
    CORRADE_ASSERT_UNREACHABLE("Text::AbstractFont::fillGlyphCache(): feature advertised but not implemented", );
}

Containers::Pointer<AbstractGlyphCache> AbstractFont::createGlyphCache() {
    CORRADE_ASSERT(isOpened(),
        "Text::AbstractFont::createGlyphCache(): no font opened", nullptr);
    CORRADE_ASSERT(features() & FontFeature::PreparedGlyphCache,
        "Text::AbstractFont::createGlyphCache(): feature not supported", nullptr);

    return doCreateGlyphCache();
}

Containers::Pointer<AbstractGlyphCache> AbstractFont::doCreateGlyphCache() {
    CORRADE_ASSERT_UNREACHABLE("Text::AbstractFont::createGlyphCache(): feature advertised but not implemented", nullptr);
}

Containers::Pointer<AbstractLayouter> AbstractFont::layout(const AbstractGlyphCache& cache, const Float size, const Containers::StringView text) {
    CORRADE_ASSERT(isOpened(), "Text::AbstractFont::layout(): no font opened", nullptr);

    return doLayout(cache, size, text);
}

Debug& operator<<(Debug& debug, const FontFeature value) {
    const bool packed = debug.immediateFlags() >= Debug::Flag::Packed;

    if(!packed)
        debug << "Text::FontFeature" << Debug::nospace;

    switch(value) {
        /* LCOV_EXCL_START */
        #define _c(v) case FontFeature::v: return debug << (packed ? "" : "::") << Debug::nospace << #v;
        _c(OpenData)
        _c(FileCallback)
        _c(PreparedGlyphCache)
        #undef _c
        /* LCOV_EXCL_STOP */
    }

    return debug << (packed ? "" : "(") << Debug::nospace << reinterpret_cast<void*>(UnsignedByte(value)) << Debug::nospace << (packed ? "" : ")");
}

Debug& operator<<(Debug& debug, const FontFeatures value) {
    return Containers::enumSetDebugOutput(debug, value, debug.immediateFlags() >= Debug::Flag::Packed ? "{}" : "Text::FontFeatures{}", {
        FontFeature::OpenData,
        FontFeature::FileCallback,
        FontFeature::PreparedGlyphCache});
}

AbstractLayouter::AbstractLayouter(UnsignedInt glyphCount): _glyphCount(glyphCount) {}

AbstractLayouter::~AbstractLayouter() = default;

Containers::Pair<Range2D, Range2D> AbstractLayouter::renderGlyph(const UnsignedInt i, Vector2& cursorPosition, Range2D& rectangle) {
    CORRADE_ASSERT(i < glyphCount(), "Text::AbstractLayouter::renderGlyph(): index" << i << "out of range for" << glyphCount() << "glyphs", {});

    /* Render the glyph */
    const Containers::Triple<Range2D, Range2D, Vector2> quadPositionTextureCoordinatesAdvance = doRenderGlyph(i);

    /* Move the quad to cursor */
    const Range2D quadPosition = quadPositionTextureCoordinatesAdvance.first().translated(cursorPosition);

    /* Extend rectangle with current quad bounds. If zero size, replace it. */
    if(!rectangle.size().isZero()) {
        rectangle.bottomLeft() = Math::min(rectangle.bottomLeft(), quadPosition.bottomLeft());
        rectangle.topRight() = Math::max(rectangle.topRight(), quadPosition.topRight());
    } else rectangle = quadPosition;

    /* Advance cursor position to next character */
    cursorPosition += quadPositionTextureCoordinatesAdvance.third();

    /* Return moved quad and unchanged texture coordinates */
    return {quadPosition, quadPositionTextureCoordinatesAdvance.second()};
}

}}
