#ifndef __C_GUI_TTFONT_H_INCLUDED__
#define __C_GUI_TTFONT_H_INCLUDED__

#include <irrlicht.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(_IRR_COMPILE_WITH_CGUITTFONT_)

namespace irr
{
namespace gui
{
    //forward added by arch_jslin 2010.06.29
    struct SGUITTFace;
	//! Structure representing a single TrueType glyph.
	struct SGUITTGlyph
	{
		//! Loads the glyph.
		void load(u32 character, FT_Face face, video::IVideoDriver* driver, u32 size,
                  bool size_is_pixels, bool fontHinting, bool autoHinting, bool useMonochrome);

		//! Unloads the glyph.
		void unload(video::IVideoDriver* driver);

		//! If true, the glyph has been loaded.
		bool isLoaded;

		//! The image data.
		//! Only used when rendering grayscale font images using the EDT_SOFTWARE driver.
		//! EDT_SOFTWARE forces A1R5G5B5 textures.  We store the IImage so we can calculate
		//! the correct alpha color.
		video::IImage* image;

		// Texture/image data.
		video::ITexture* texture;

		// Bitmap information.
		core::rect<u32> bitmap_size;

		// Texture information.
		core::dimension2du texture_size;

		// Glyph advance information.
		FT_Vector advance;
	};

//! Class representing a TrueType font.
class CGUITTFont : public IGUITTFont
{
public:
    //! Creates a new TrueType font and returns a pointer to it.
    //! The pointer must be drop()'ed when finished.
    //! \param env The IGUIEnvironment the font loads out of.
    //! \param filename The filename of the font.
    //! \param size The size of the font.
    //! \param size_in_pixel If true, size is represented as pixels instead of points.
    //! \return Returns a pointer to a CGUITTFont.  Will return 0 if the font failed to load.
    static CGUITTFont* create(IGUIEnvironment *env, const io::path& filename, const u32 size,
                              const bool size_in_pixel = false);

    //! Destructor
    virtual ~CGUITTFont();

    //! Get the font size (default unit is point).
    virtual u32 getFontSize() const { return size; }

    //! Check the unit of size (in pixel or in point).
    virtual bool isSizeInPixel() const { return size_in_pixel; }

    //! Check the font's transparency.
    virtual bool isTransparent() const { return use_transparency; }

    //! Check if the font auto-hinting is enabled.
    virtual bool useAutoHinting() const { return use_auto_hinting; }

    //NOTE: I don't know what's the difference between auto-hinting / hinting.
    //! Check if the font hinting is enabled.
    virtual bool useHinting()     const { return use_hinting; }

    //! Check if the font is monochrome.
    //! This is opposed to the "anti-alias" concept.
    virtual bool useMonochrome()  const { return use_monochrome; }

    //! Tells the font to allow transparency when rendering.
    //! Default: true.
    //! \param flag If true, the font draws using transparency.
    virtual void setTransparency(const bool flag) { use_transparency = flag; }

    //! Tells the font to use monochrome rendering.
    //! Default: false.
    //! \param flag If true, the font draws using a monochrome image.
    //!        If false, the font uses a grayscale image.
    virtual void setMonochrome(const bool flag);

    //! Enables or disables font hinting.
    //! Default: Hinting and auto-hinting true.
    //! \param enable If false, font hinting is turned off. If true, font hinting is turned on.
    //! \param enable_auto_hinting If true, FreeType uses its own auto-hinting algorithm.
    //!        If false, it tries to use the algorithm specified by the font.
    virtual void setFontHinting(const bool enable, const bool enable_auto_hinting = true);

    //! Draws some text and clips it to the specified rectangle if wanted.
    virtual void draw(const core::stringw& text, const core::rect<s32>& position,
        video::SColor color, bool hcenter=false, bool vcenter=false,
        const core::rect<s32>* clip=0);

    //! Returns the dimension of a text string.
    virtual core::dimension2d<u32> getDimension(const wchar_t* text) const;
    virtual core::dimension2d<u32> getDimension(const core::ustring& text) const;

    //! Calculates the index of the character in the text which is on a specific position.
    virtual s32 getCharacterFromPos(const wchar_t* text, s32 pixel_x) const;
    virtual s32 getCharacterFromPos(const core::ustring& text, s32 pixel_x) const;

    //! Sets global kerning width for the font.
    virtual void setKerningWidth(s32 kerning);

    //! Sets global kerning height for the font.
    virtual void setKerningHeight(s32 kerning);

    //! Gets kerning values (distance between letters) for the font. If no parameters are provided,
    virtual s32 getKerningWidth(const wchar_t* thisLetter=0, const wchar_t* previousLetter=0) const;
    virtual s32 getKerningWidth(const uchar32_t thisLetter=0, const uchar32_t previousLetter=0) const;

    //! Returns the distance between letters
    virtual s32 getKerningHeight() const;

    //! Define which characters should not be drawn by the font.
    virtual void setInvisibleCharacters(const wchar_t *s);
    virtual void setInvisibleCharacters(const core::ustring& s);

    //! Get corresponding irrlicht video texture from the font,
    //! so you can use this font just like any ordinary texture.
    virtual video::ITexture* getTextureFromChar(const wchar_t ch);

protected:
// properties moved to protected scope instead of public.
    bool use_monochrome;
    bool use_transparency;
    bool use_hinting;
    bool use_auto_hinting;
    u32  size;
    bool size_in_pixel;

private:
    // Manages the FreeType library.
    static FT_Library c_library;
    static core::map<io::path, SGUITTFace*> c_faces;
    static bool c_libraryLoaded;

    CGUITTFont(IGUIEnvironment *env);
    bool load(const io::path& filename, const u32 size, const bool size_in_pixel = false);

    u32 getWidthFromCharacter(wchar_t c) const;
    u32 getWidthFromCharacter(uchar32_t c) const;
    u32 getHeightFromCharacter(wchar_t c) const;
    u32 getHeightFromCharacter(uchar32_t c) const;
    u32 getGlyphIndexByChar(wchar_t c) const;
    u32 getGlyphIndexByChar(uchar32_t c) const;
    core::vector2di getKerning(const wchar_t thisLetter, const wchar_t previousLetter) const;
    core::vector2di getKerning(const uchar32_t thisLetter, const uchar32_t previousLetter) const;

    gui::IGUIEnvironment* Environment;
    video::IVideoDriver* Driver;
    io::path filename;
    FT_Face tt_face;

    mutable core::array<SGUITTGlyph> Glyphs;

    s32 GlobalKerningWidth;
    s32 GlobalKerningHeight;
    core::ustring Invisible;
};

} // end namespace gui
} // end namespace irr

#endif //if _IRR_COMPILE_WITH_CGUITTFONT_

#endif // __C_GUI_TTFONT_H_INCLUDED__

