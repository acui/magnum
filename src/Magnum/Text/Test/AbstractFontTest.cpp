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

#include <sstream>
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/String.h>
#include <Corrade/Containers/StringStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Containers/Triple.h>
#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Container.h>
#include <Corrade/TestSuite/Compare/String.h>
#include <Corrade/Utility/DebugStl.h> /** @todo remove once Debug is stream-free */
#include <Corrade/Utility/Path.h>

#include "Magnum/FileCallback.h"
#include "Magnum/PixelFormat.h"
#include "Magnum/Math/Range.h"
#include "Magnum/Math/Vector2.h"
#include "Magnum/Text/AbstractFont.h"
#include "Magnum/Text/AbstractGlyphCache.h"

#include "configure.h"

namespace Magnum { namespace Text { namespace Test { namespace {

struct AbstractFontTest: TestSuite::Tester {
    explicit AbstractFontTest();

    void construct();

    void openData();
    void openFileAsData();
    void openFileAsDataNotFound();

    void openFileNotImplemented();
    void openDataNotSupported();
    void openDataNotImplemented();

    void setFileCallback();
    void setFileCallbackTemplate();
    void setFileCallbackTemplateNull();
    void setFileCallbackTemplateConst();
    void setFileCallbackFileOpened();
    void setFileCallbackNotImplemented();
    void setFileCallbackNotSupported();
    void setFileCallbackOpenFileDirectly();
    void setFileCallbackOpenFileThroughBaseImplementation();
    void setFileCallbackOpenFileThroughBaseImplementationFailed();
    void setFileCallbackOpenFileAsData();
    void setFileCallbackOpenFileAsDataFailed();

    void properties();
    void propertiesNoFont();

    void glyphId();
    void glyphIdNoFont();

    void glyphSizeAdvance();
    void glyphSizeAdvanceNoFont();
    void glyphSizeAdvanceOutOfRange();

    void layout();
    void layoutNoFont();

    void fillGlyphCache();
    void fillGlyphCacheNotSupported();
    void fillGlyphCacheNotImplemented();
    void fillGlyphCacheNoFont();
    void fillGlyphCacheInvalidUtf8();

    void createGlyphCache();
    void createGlyphCacheNotSupported();
    void createGlyphCacheNotImplemented();
    void createGlyphCacheNoFont();

    void debugFeature();
    void debugFeaturePacked();
    void debugFeatures();
    void debugFeaturesPacked();
};

AbstractFontTest::AbstractFontTest() {
    addTests({&AbstractFontTest::construct,

              &AbstractFontTest::openData,
              &AbstractFontTest::openFileAsData,
              &AbstractFontTest::openFileAsDataNotFound,

              &AbstractFontTest::openFileNotImplemented,
              &AbstractFontTest::openDataNotSupported,
              &AbstractFontTest::openDataNotImplemented,

              &AbstractFontTest::setFileCallback,
              &AbstractFontTest::setFileCallbackTemplate,
              &AbstractFontTest::setFileCallbackTemplateNull,
              &AbstractFontTest::setFileCallbackTemplateConst,
              &AbstractFontTest::setFileCallbackFileOpened,
              &AbstractFontTest::setFileCallbackNotImplemented,
              &AbstractFontTest::setFileCallbackNotSupported,
              &AbstractFontTest::setFileCallbackOpenFileDirectly,
              &AbstractFontTest::setFileCallbackOpenFileThroughBaseImplementation,
              &AbstractFontTest::setFileCallbackOpenFileThroughBaseImplementationFailed,
              &AbstractFontTest::setFileCallbackOpenFileAsData,
              &AbstractFontTest::setFileCallbackOpenFileAsDataFailed,

              &AbstractFontTest::properties,
              &AbstractFontTest::propertiesNoFont,

              &AbstractFontTest::glyphId,
              &AbstractFontTest::glyphIdNoFont,

              &AbstractFontTest::glyphSizeAdvance,
              &AbstractFontTest::glyphSizeAdvanceNoFont,
              &AbstractFontTest::glyphSizeAdvanceOutOfRange,

              &AbstractFontTest::layout,
              &AbstractFontTest::layoutNoFont,

              &AbstractFontTest::fillGlyphCache,
              &AbstractFontTest::fillGlyphCacheNotSupported,
              &AbstractFontTest::fillGlyphCacheNotImplemented,
              &AbstractFontTest::fillGlyphCacheNoFont,
              &AbstractFontTest::fillGlyphCacheInvalidUtf8,

              &AbstractFontTest::createGlyphCache,
              &AbstractFontTest::createGlyphCacheNotSupported,
              &AbstractFontTest::createGlyphCacheNotImplemented,
              &AbstractFontTest::createGlyphCacheNoFont,

              &AbstractFontTest::debugFeature,
              &AbstractFontTest::debugFeaturePacked,
              &AbstractFontTest::debugFeatures,
              &AbstractFontTest::debugFeaturesPacked});
}

using namespace Containers::Literals;

void AbstractFontTest::construct() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    CORRADE_COMPARE(font.features(), FontFeatures{});
    CORRADE_VERIFY(!font.isOpened());

    font.close();
    CORRADE_VERIFY(!font.isOpened());
}

void AbstractFontTest::openData() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData; }
        bool doIsOpened() const override { return _opened; }
        void doClose() override {}

        Properties doOpenData(const Containers::ArrayView<const char> data, Float size) override {
            _opened = (data.size() == 1 && data[0] == '\xa5');
            return {size, 1.0f, 2.0f, 3.0f, 15};
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool _opened = false;
    } font;

    CORRADE_VERIFY(!font.isOpened());
    const char a5 = '\xa5';
    font.openData({&a5, 1}, 13.0f);
    CORRADE_VERIFY(font.isOpened());
    CORRADE_COMPARE(font.size(), 13.0f);
    CORRADE_COMPARE(font.ascent(), 1.0f);
    CORRADE_COMPARE(font.descent(), 2.0f);
    CORRADE_COMPARE(font.lineHeight(), 3.0f);
    CORRADE_COMPARE(font.glyphCount(), 15);
}

void AbstractFontTest::openFileAsData() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData; }
        bool doIsOpened() const override { return _opened; }
        void doClose() override {}

        Properties doOpenData(const Containers::ArrayView<const char> data, Float size) override {
            _opened = (data.size() == 1 && data[0] == '\xa5');
            return {size, 1.0f, 2.0f, 3.0f, 15};
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool _opened = false;
    } font;

    /* doOpenFile() should call doOpenData() */
    CORRADE_VERIFY(!font.isOpened());
    font.openFile(Utility::Path::join(TEXT_TEST_DIR, "data.bin"), 13.0f);
    CORRADE_VERIFY(font.isOpened());
    CORRADE_COMPARE(font.size(), 13.0f);
    CORRADE_COMPARE(font.ascent(), 1.0f);
    CORRADE_COMPARE(font.descent(), 2.0f);
    CORRADE_COMPARE(font.lineHeight(), 3.0f);
    CORRADE_COMPARE(font.glyphCount(), 15);
}

void AbstractFontTest::openFileAsDataNotFound() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    CORRADE_VERIFY(!font.openFile("nonexistent.foo", 12.0f));
    /* There's an error message from Path::read() before */
    CORRADE_COMPARE_AS(out.str(),
        "\nText::AbstractFont::openFile(): cannot open file nonexistent.foo\n",
        TestSuite::Compare::StringHasSuffix);
}

void AbstractFontTest::openFileNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractFont {
        /* Supports neither file nor data opening */
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    font.openFile("file.foo", 34.0f);
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::openFile(): not implemented\n");
}

void AbstractFontTest::openDataNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractFont {
        /* Supports neither file nor data opening */
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    font.openData(nullptr, 34.0f);
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::openData(): feature not supported\n");
}

void AbstractFontTest::openDataNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    font.openData(nullptr, 34.0f);
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::openData(): feature advertised but not implemented\n");
}

void AbstractFontTest::setFileCallback() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData|FontFeature::FileCallback; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}
        void doSetFileCallback(Containers::Optional<Containers::ArrayView<const char>>(*)(const std::string&, InputFileCallbackPolicy, void*), void* userData) override {
            *static_cast<int*>(userData) = 1337;
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    int a = 0;
    auto lambda = [](const std::string&, InputFileCallbackPolicy, void*) {
        return Containers::Optional<Containers::ArrayView<const char>>{};
    };
    font.setFileCallback(lambda, &a);
    CORRADE_COMPARE(font.fileCallback(), lambda);
    CORRADE_COMPARE(font.fileCallbackUserData(), &a);
    CORRADE_COMPARE(a, 1337);
}

void AbstractFontTest::setFileCallbackTemplate() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData|FontFeature::FileCallback; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}
        void doSetFileCallback(Containers::Optional<Containers::ArrayView<const char>>(*)(const std::string&, InputFileCallbackPolicy, void*), void*) override {
            called = true;
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool called = false;
    } font;

    int a = 0;
    auto lambda = [](const std::string&, InputFileCallbackPolicy, int&) {
        return Containers::Optional<Containers::ArrayView<const char>>{};
    };
    font.setFileCallback(lambda, a);
    CORRADE_VERIFY(font.fileCallback());
    CORRADE_VERIFY(font.fileCallbackUserData());
    CORRADE_VERIFY(font.called);

    /* The data pointers should be wrapped, thus not the same */
    CORRADE_VERIFY(reinterpret_cast<void(*)()>(font.fileCallback()) != reinterpret_cast<void(*)()>(static_cast<Containers::Optional<Containers::ArrayView<const char>>(*)(const std::string&, InputFileCallbackPolicy, int&)>(lambda)));
    CORRADE_VERIFY(font.fileCallbackUserData() != &a);
}

void AbstractFontTest::setFileCallbackTemplateNull() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData|FontFeature::FileCallback; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}
        void doSetFileCallback(Containers::Optional<Containers::ArrayView<const char>>(*callback)(const std::string&, InputFileCallbackPolicy, void*), void* userData) override {
            called = !callback && !userData;
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool called = false;
    } font;

    int a = 0;
    font.setFileCallback(static_cast<Containers::Optional<Containers::ArrayView<const char>>(*)(const std::string&, InputFileCallbackPolicy, int&)>(nullptr), a);
    CORRADE_VERIFY(!font.fileCallback());
    CORRADE_VERIFY(!font.fileCallbackUserData());
    CORRADE_VERIFY(font.called);
}

void AbstractFontTest::setFileCallbackTemplateConst() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData|FontFeature::FileCallback; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}
        void doSetFileCallback(Containers::Optional<Containers::ArrayView<const char>>(*)(const std::string&, InputFileCallbackPolicy, void*), void*) override {
            called = true;
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool called = false;
    } font;

    const int a = 0;
    auto lambda = [](const std::string&, InputFileCallbackPolicy, const int&) {
        return Containers::Optional<Containers::ArrayView<const char>>{};
    };
    font.setFileCallback(lambda, a);
    CORRADE_VERIFY(font.fileCallback());
    CORRADE_VERIFY(font.fileCallbackUserData());
    CORRADE_VERIFY(font.called);
}

void AbstractFontTest::setFileCallbackFileOpened() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    std::ostringstream out;
    Error redirectError{&out};

    font.setFileCallback([](const std::string&, InputFileCallbackPolicy, void*) {
        return Containers::Optional<Containers::ArrayView<const char>>{};
    });
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::setFileCallback(): can't be set while a font is opened\n");
}

void AbstractFontTest::setFileCallbackNotImplemented() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::FileCallback; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    int a;
    auto lambda = [](const std::string&, InputFileCallbackPolicy, void*) {
        return Containers::Optional<Containers::ArrayView<const char>>{};
    };
    font.setFileCallback(lambda, &a);
    CORRADE_COMPARE(font.fileCallback(), lambda);
    CORRADE_COMPARE(font.fileCallbackUserData(), &a);
    /* Should just work, no need to implement the function */
}

void AbstractFontTest::setFileCallbackNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    std::ostringstream out;
    Error redirectError{&out};

    int a;
    font.setFileCallback([](const std::string&, InputFileCallbackPolicy, void*) {
        return Containers::Optional<Containers::ArrayView<const char>>{};
    }, &a);
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::setFileCallback(): font plugin supports neither loading from data nor via callbacks, callbacks can't be used\n");
}

void AbstractFontTest::setFileCallbackOpenFileDirectly() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::FileCallback|FontFeature::OpenData; }
        bool doIsOpened() const override { return _opened; }
        void doClose() override { _opened = false; }

        Properties doOpenFile(Containers::StringView filename, Float size) override {
            /* Called because FileCallback is supported */
            _opened = filename == "file.dat" && fileCallback() && fileCallbackUserData();
            return {size, 1.0f, 2.0f, 3.0f, 15};
        }

        Properties doOpenData(Containers::ArrayView<const char>, Float) override {
            /* Shouldn't be called because FileCallback is supported */
            openDataCalledNotSureWhy = true;
            return {};
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool _opened = false;
        bool openDataCalledNotSureWhy = false;
    } font;

    bool calledNotSureWhy = false;
    font.setFileCallback([](const std::string&, InputFileCallbackPolicy, bool& calledNotSureWhy) -> Containers::Optional<Containers::ArrayView<const char>> {
        calledNotSureWhy = true;
        return {};
    }, calledNotSureWhy);

    CORRADE_VERIFY(font.openFile("file.dat", 42.0f));
    CORRADE_VERIFY(!calledNotSureWhy);
    CORRADE_VERIFY(!font.openDataCalledNotSureWhy);
    CORRADE_COMPARE(font.size(), 42.0f);
    CORRADE_COMPARE(font.ascent(), 1.0f);
    CORRADE_COMPARE(font.descent(), 2.0f);
    CORRADE_COMPARE(font.lineHeight(), 3.0f);
    CORRADE_COMPARE(font.glyphCount(), 15);
}

void AbstractFontTest::setFileCallbackOpenFileThroughBaseImplementation() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::FileCallback|FontFeature::OpenData; }
        bool doIsOpened() const override { return _opened; }
        void doClose() override { _opened = false; }

        Properties doOpenFile(Containers::StringView filename, Float size) override {
            openFileCalled = filename == "file.dat" && fileCallback() && fileCallbackUserData();
            return AbstractFont::doOpenFile(filename, size);
        }

        Properties doOpenData(Containers::ArrayView<const char> data, Float size) override {
            _opened = (data.size() == 1 && data[0] == '\xb0');
            return {size, 1.0f, 2.0f, 3.0f, 15};
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool _opened = false;
        bool openFileCalled = false;
    } font;

    struct State {
        const char data = '\xb0';
        bool loaded = false;
        bool closed = false;
        bool calledNotSureWhy = false;
    } state;
    font.setFileCallback([](const std::string& filename, InputFileCallbackPolicy policy, State& state) -> Containers::Optional<Containers::ArrayView<const char>> {
        if(filename == "file.dat" && policy == InputFileCallbackPolicy::LoadTemporary) {
            state.loaded = true;
            return Containers::arrayView(&state.data, 1);
        }

        if(filename == "file.dat" && policy == InputFileCallbackPolicy::Close) {
            state.closed = true;
            return {};
        }

        state.calledNotSureWhy = true;
        return {};
    }, state);

    CORRADE_VERIFY(font.openFile("file.dat", 42.0f));
    CORRADE_VERIFY(font.openFileCalled);
    CORRADE_VERIFY(state.loaded);
    CORRADE_VERIFY(state.closed);
    CORRADE_VERIFY(!state.calledNotSureWhy);
    CORRADE_COMPARE(font.size(), 42.0f);
    CORRADE_COMPARE(font.ascent(), 1.0f);
    CORRADE_COMPARE(font.descent(), 2.0f);
    CORRADE_COMPARE(font.lineHeight(), 3.0f);
    CORRADE_COMPARE(font.glyphCount(), 15);
}

void AbstractFontTest::setFileCallbackOpenFileThroughBaseImplementationFailed() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::FileCallback|FontFeature::OpenData; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        Properties doOpenFile(Containers::StringView filename, Float size) override {
            openFileCalled = true;
            return AbstractFont::doOpenFile(filename, size);
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

         bool openFileCalled = false;
    } font;

    font.setFileCallback([](const std::string&, InputFileCallbackPolicy, void*) -> Containers::Optional<Containers::ArrayView<const char>> {
        return {};
    });

    std::ostringstream out;
    Error redirectError{&out};

    CORRADE_VERIFY(!font.openFile("file.dat", 42.0f));
    CORRADE_VERIFY(font.openFileCalled);
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::openFile(): cannot open file file.dat\n");
}

void AbstractFontTest::setFileCallbackOpenFileAsData() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData; }
        bool doIsOpened() const override { return _opened; }
        void doClose() override { _opened = false; }

        Properties doOpenFile(Containers::StringView, Float) override {
            openFileCalled = true;
            return {};
        }

        Properties doOpenData(Containers::ArrayView<const char> data, Float size) override {
            _opened = (data.size() == 1 && data[0] == '\xb0');
            return {size, 1.0f, 2.0f, 3.0f, 15};
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool _opened = false;
        bool openFileCalled = false;
    } font;

    struct State {
        const char data = '\xb0';
        bool loaded = false;
        bool closed = false;
        bool calledNotSureWhy = false;
    } state;

    font.setFileCallback([](const std::string& filename, InputFileCallbackPolicy policy, State& state) -> Containers::Optional<Containers::ArrayView<const char>> {
        if(filename == "file.dat" && policy == InputFileCallbackPolicy::LoadTemporary) {
            state.loaded = true;
            return Containers::arrayView(&state.data, 1);
        }

        if(filename == "file.dat" && policy == InputFileCallbackPolicy::Close) {
            state.closed = true;
            return {};
        }

        state.calledNotSureWhy = true;
        return {};
    }, state);

    CORRADE_VERIFY(font.openFile("file.dat", 13.0f));
    CORRADE_VERIFY(!font.openFileCalled);
    CORRADE_VERIFY(state.loaded);
    CORRADE_VERIFY(state.closed);
    CORRADE_VERIFY(!state.calledNotSureWhy);
    CORRADE_COMPARE(font.size(), 13.0f);
    CORRADE_COMPARE(font.ascent(), 1.0f);
    CORRADE_COMPARE(font.descent(), 2.0f);
    CORRADE_COMPARE(font.lineHeight(), 3.0f);
    CORRADE_COMPARE(font.glyphCount(), 15);
}

void AbstractFontTest::setFileCallbackOpenFileAsDataFailed() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        Properties doOpenFile(Containers::StringView, Float) override {
            openFileCalled = true;
            return {};
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool openFileCalled = false;
    } font;

    font.setFileCallback([](const std::string&, InputFileCallbackPolicy, void*) {
        return Containers::Optional<Containers::ArrayView<const char>>{};
    });

    std::ostringstream out;
    Error redirectError{&out};

    CORRADE_VERIFY(!font.openFile("file.dat", 132.0f));
    CORRADE_VERIFY(!font.openFileCalled);
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::openFile(): cannot open file file.dat\n");
}

void AbstractFontTest::properties() {
    struct: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData; }
        bool doIsOpened() const override { return _opened; }
        void doClose() override {}

        Properties doOpenData(const Containers::ArrayView<const char>, Float size) override {
            _opened = true;
            return {size, 1.0f, 2.0f, 3.0f, 15};
        }

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool _opened = false;
    } font;

    CORRADE_VERIFY(font.openData(nullptr, 13.0f));
    CORRADE_COMPARE(font.size(), 13.0f);
    CORRADE_COMPARE(font.ascent(), 1.0f);
    CORRADE_COMPARE(font.descent(), 2.0f);
    CORRADE_COMPARE(font.lineHeight(), 3.0f);
    CORRADE_COMPARE(font.glyphCount(), 15);
}

void AbstractFontTest::propertiesNoFont() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    font.size();
    font.ascent();
    font.descent();
    font.lineHeight();
    font.glyphCount();
    CORRADE_COMPARE(out.str(),
        "Text::AbstractFont::size(): no font opened\n"
        "Text::AbstractFont::ascent(): no font opened\n"
        "Text::AbstractFont::descent(): no font opened\n"
        "Text::AbstractFont::lineHeight(): no font opened\n"
        "Text::AbstractFont::glyphCount(): no font opened\n");
}

void AbstractFontTest::glyphId() {
    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t a) override { return a*10; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    CORRADE_COMPARE(font.glyphId(U'a'), 970);
}

void AbstractFontTest::glyphIdNoFont() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    font.glyphId('a');
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::glyphId(): no font opened\n");
}

void AbstractFontTest::glyphSizeAdvance() {
    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData; }
        bool doIsOpened() const override { return _opened; }
        void doClose() override {}

        Properties doOpenData(Containers::ArrayView<const char>, Float) override {
            _opened = true;
            return {0.0f, 0.0f, 0.0f, 0.0f, 98};
        }
        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt a) override { return {a*2.0f, a/3.0f}; }
        Vector2 doGlyphAdvance(UnsignedInt a) override { return {a*10.0f, -Float(a)/10.0f}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool _opened = false;
    } font;

    /* Have to explicitly open in order to make glyphCount() non-zero */
    CORRADE_VERIFY(font.openData(nullptr, 0.0f));
    CORRADE_COMPARE(font.glyphSize(33), (Vector2{66.0f, 11.0f}));
    CORRADE_COMPARE(font.glyphAdvance(97), (Vector2{970.0f, -9.7f}));
}

void AbstractFontTest::glyphSizeAdvanceNoFont() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    font.glyphSize(33);
    font.glyphAdvance(97);
    CORRADE_COMPARE(out.str(),
        "Text::AbstractFont::glyphSize(): no font opened\n"
        "Text::AbstractFont::glyphAdvance(): no font opened\n");
}

void AbstractFontTest::glyphSizeAdvanceOutOfRange() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::OpenData; }
        bool doIsOpened() const override { return _opened; }
        void doClose() override {}

        Properties doOpenData(Containers::ArrayView<const char>, Float) override {
            _opened = true;
            return {0.0f, 0.0f, 0.0f, 0.0f, 3};
        }
        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override {
            return nullptr;
        }

        bool _opened = false;
    } font;

    /* Have to explicitly open in order to make glyphCount() non-zero */
    CORRADE_VERIFY(font.openData(nullptr, 0.0f));

    std::ostringstream out;
    Error redirectError{&out};
    font.glyphSize(3);
    font.glyphAdvance(3);
    CORRADE_COMPARE(out.str(),
        "Text::AbstractFont::glyphSize(): index 3 out of range for 3 glyphs\n"
        "Text::AbstractFont::glyphAdvance(): index 3 out of range for 3 glyphs\n");
}

struct DummyGlyphCache: AbstractGlyphCache {
    using AbstractGlyphCache::AbstractGlyphCache;

    GlyphCacheFeatures doFeatures() const override { return {}; }
    void doSetImage(const Vector2i&, const ImageView2D&) override {}
};

void AbstractFontTest::layout() {
    struct Layouter: AbstractLayouter {
        explicit Layouter(UnsignedInt count): AbstractLayouter{count} {}
        Containers::Triple<Range2D, Range2D, Vector2> doRenderGlyph(UnsignedInt) override { return {}; }
    };

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache& cache, Float size, Containers::StringView str) override {
            return Containers::pointer<Layouter>(UnsignedInt(cache.size().x()*str.size()*size));
        }
    } font;

    DummyGlyphCache cache{PixelFormat::R8Unorm, {100, 200}};
    Containers::Pointer<AbstractLayouter> layouter = font.layout(cache, 0.25f, "hello");
    CORRADE_COMPARE(layouter->glyphCount(), 100*5/4);
}

void AbstractFontTest::layoutNoFont() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    DummyGlyphCache cache{PixelFormat::R8Unorm, {100, 200}};
    font.layout(cache, 0.25f, "hello");
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::layout(): no font opened\n");
}

void AbstractFontTest::fillGlyphCache() {
    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }

        void doFillGlyphCache(AbstractGlyphCache& cache, Containers::ArrayView<const char32_t> characters) override {
            CORRADE_COMPARE(cache.size(), (Vector3i{100, 100, 1}));
            CORRADE_COMPARE_AS(characters, Containers::arrayView<char32_t>({
                'h', 'e', 'l', 'o'
            }), TestSuite::Compare::Container);
            called = true;
        }

        bool called = false;
    } font;

    /* Capture correct function name */
    CORRADE_VERIFY(true);

    DummyGlyphCache cache{PixelFormat::R8Unorm, {100, 100}};

    font.fillGlyphCache(cache, "helo");
    CORRADE_VERIFY(font.called);
}

void AbstractFontTest::fillGlyphCacheNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::PreparedGlyphCache; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    DummyGlyphCache cache{PixelFormat::R8Unorm, {100, 100}};
    font.fillGlyphCache(cache, "hello");
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::fillGlyphCache(): feature not supported\n");
}

void AbstractFontTest::fillGlyphCacheNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    DummyGlyphCache cache{PixelFormat::R8Unorm, {100, 100}};
    font.fillGlyphCache(cache, "hello");
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::fillGlyphCache(): feature advertised but not implemented\n");
}

void AbstractFontTest::fillGlyphCacheNoFont() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    DummyGlyphCache cache{PixelFormat::R8Unorm, {100, 100}};
    font.fillGlyphCache(cache, "hello");
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::fillGlyphCache(): no font opened\n");
}

void AbstractFontTest::fillGlyphCacheInvalidUtf8() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    DummyGlyphCache cache{PixelFormat::R8Unorm, {100, 100}};
    font.fillGlyphCache(cache, "he\xffo");
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::fillGlyphCache(): not a valid UTF-8 string: he\xffo\n");
}

void AbstractFontTest::createGlyphCache() {
    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::PreparedGlyphCache; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }

        Containers::Pointer<AbstractGlyphCache> doCreateGlyphCache() override {
            return Containers::pointer<DummyGlyphCache>(PixelFormat::R8Unorm, Vector2i{123, 345});
        }
    } font;

    Containers::Pointer<AbstractGlyphCache> cache = font.createGlyphCache();
    CORRADE_VERIFY(cache);

    CORRADE_COMPARE(cache->size(), (Vector3i{123, 345, 1}));
}

void AbstractFontTest::createGlyphCacheNotSupported() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return {}; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    font.createGlyphCache();
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::createGlyphCache(): feature not supported\n");
}

void AbstractFontTest::createGlyphCacheNotImplemented() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::PreparedGlyphCache; }
        bool doIsOpened() const override { return true; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    font.createGlyphCache();
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::createGlyphCache(): feature advertised but not implemented\n");
}

void AbstractFontTest::createGlyphCacheNoFont() {
    CORRADE_SKIP_IF_NO_ASSERT();

    struct MyFont: AbstractFont {
        FontFeatures doFeatures() const override { return FontFeature::PreparedGlyphCache; }
        bool doIsOpened() const override { return false; }
        void doClose() override {}

        UnsignedInt doGlyphId(char32_t) override { return {}; }
        Vector2 doGlyphSize(UnsignedInt) override { return {}; }
        Vector2 doGlyphAdvance(UnsignedInt) override { return {}; }
        Containers::Pointer<AbstractLayouter> doLayout(const AbstractGlyphCache&, Float, Containers::StringView) override { return nullptr; }
    } font;

    std::ostringstream out;
    Error redirectError{&out};
    font.createGlyphCache();
    CORRADE_COMPARE(out.str(), "Text::AbstractFont::createGlyphCache(): no font opened\n");
}

void AbstractFontTest::debugFeature() {
    std::ostringstream out;

    Debug{&out} << FontFeature::OpenData << FontFeature(0xf0);
    CORRADE_COMPARE(out.str(), "Text::FontFeature::OpenData Text::FontFeature(0xf0)\n");
}

void AbstractFontTest::debugFeaturePacked() {
    std::ostringstream out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << FontFeature::OpenData << Debug::packed << FontFeature(0xf0) << FontFeature::FileCallback;
    CORRADE_COMPARE(out.str(), "OpenData 0xf0 Text::FontFeature::FileCallback\n");
}

void AbstractFontTest::debugFeatures() {
    std::ostringstream out;

    Debug{&out} << (FontFeature::OpenData|FontFeature::PreparedGlyphCache) << FontFeatures{};
    CORRADE_COMPARE(out.str(), "Text::FontFeature::OpenData|Text::FontFeature::PreparedGlyphCache Text::FontFeatures{}\n");
}

void AbstractFontTest::debugFeaturesPacked() {
    std::ostringstream out;
    /* Last is not packed, ones before should not make any flags persistent */
    Debug{&out} << Debug::packed << (FontFeature::OpenData|FontFeature::PreparedGlyphCache) << Debug::packed << FontFeatures{} << FontFeature::FileCallback;
    CORRADE_COMPARE(out.str(), "OpenData|PreparedGlyphCache {} Text::FontFeature::FileCallback\n");
}

}}}}

CORRADE_TEST_MAIN(Magnum::Text::Test::AbstractFontTest)
