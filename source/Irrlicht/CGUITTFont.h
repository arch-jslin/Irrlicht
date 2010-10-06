#ifndef __C_GUI_TTFONT_H_INCLUDED__
#define __C_GUI_TTFONT_H_INCLUDED__

#if defined(_IRR_COMPILE_WITH_CGUITTFONT_)

#include <irrlicht.h>
#include <ft2build.h>
#include FT_FREETYPE_H

//Last updated by arch.jslin 2010.07.21:
//   * Refactored the glyph loading part, so it doesn't need an additional CImage
//     object other than the Texture itself.
//   * modified CGUITTFont::createImageFromChar and CGUITTFont::getPageTextureByIndex

namespace irr
{
namespace gui
{

struct SGUITTFace;
class  CGUITTFont; // Because now SGUITTGlyph holds a pointer to its parent (CGUITTFont)
                   // It has to be forward declared.
// Assists us in deleting Glyphs.
class CGUITTAssistDelete
{
public:
    template <class T, typename TAlloc>
    static void Delete(core::array<T, TAlloc>& a)
    {
        TAlloc allocator;
        allocator.deallocate(a.pointer());
    }
};

//! Structure representing a single TrueType glyph.
struct SGUITTGlyph
{
    //! Preload the glyph:
    //! Preload process occurs when the program tries to cache the glyph from FT_Library,
    //! however, it simply defines the SGUITTGlyph's properties, and will create page textures
    //! if nessesary. The glyph bitmap will be cached temporarily, but after the actual copy to
    //! texture process, that image data will be dropped. (See CGUITTGlyphPage)
    void preload(const u32& char_index, FT_Face face, video::IVideoDriver* driver,
                 const u32& font_size, const FT_Int32& loadFlags);

    //! Unloads the glyph.
    void unload();

    //! If true, the glyph has been loaded.
    bool isLoaded;

    //! The page the glyph is on.
    u32 glyph_page;

    //! The source rectangle for the glyph.
    core::recti source_rect;

    //! The offset of glyph when drawn.
    core::vector2di offset;

    //! Glyph advance information.
    FT_Vector advance;

    //! This is just the temporary image holder. after this glyph is paged,
    //! it will be dropped.
    mutable video::IImage* surface;

    //! The pointer pointing to the parent (CGUITTFont)
    CGUITTFont* parent;

private:
    //! create the Image object from the FT_Bitmap.
    //! This is called in the preload process, and should not be called anywhere again for the
    //! same glyph.
    video::IImage* createGlyphImage(const FT_Bitmap& bits, video::IVideoDriver* driver) const;
};

//! Holds a sheet of glyphs.
class CGUITTGlyphPage
{
public:
    CGUITTGlyphPage(video::IVideoDriver* Driver, const io::path& texture_name)
        :texture(0), available_slots(0), used_slots(0), dirty(false), driver(Driver), name(texture_name) {}
    ~CGUITTGlyphPage()
    {
        if (texture)
        {
            if (driver)
                driver->removeTexture(texture);
            else texture->drop();
        }
    }

    //! Create the actual page texture.
    //! \param pixel_mode The FT_Pixel_Mode value specifing what format the bitmap uses.
    //! \param texture_size specify the size of this page texture.
    bool createPageTexture(const FT_Pixel_Mode& pixel_mode, const core::dimension2du& texture_size)
    {
        if( texture )
            return false;

        bool flgmip = driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
        driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

        // Set the texture color format
        switch (pixel_mode)
        {
            case FT_PIXEL_MODE_MONO:
                texture = driver->addTexture(texture_size, name, video::ECF_A1R5G5B5);
                break;
            case FT_PIXEL_MODE_GRAY:
            default:
                texture = driver->addTexture(texture_size, name, video::ECF_A8R8G8B8);
                break;
        }
        // Restore our texture creation flags.
        driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, flgmip);
        return texture ? true : false;
    }

    //! add the glyph to the collection, used when updateTexture is called.
    //! this collection will be cleared after updateTexture is called.
    //! \param glyph Pointer to the glyph to be paged.
    void pushGlyphToBePaged(const SGUITTGlyph* glyph)
    {
        glyph_to_be_paged.push_back(glyph);
    }

    //! This method should only be called when the texture is dirty.
    //! It blits the image of recorded glyphs on to the video texture.
    void updateTexture()
    {
        void* ptr                   = texture->lock();
        video::ECOLOR_FORMAT format = texture->getColorFormat();
        core::dimension2du size     = texture->getOriginalSize();
        video::IImage* pageholder   = driver->createImageFromData(format, size, ptr, true, false);

        for(u32 i = 0; i < glyph_to_be_paged.size(); ++i) {
            const SGUITTGlyph* glyph = glyph_to_be_paged[i];
            if( glyph && glyph->isLoaded ) {
                if( glyph->surface ) {
                    glyph->surface->copyTo(pageholder, glyph->source_rect.UpperLeftCorner);
                    glyph->surface->drop();
                    glyph->surface = 0;  //NOTICE: because of this, surface has to be mutable
                } else {
                    ; // TODO: add error message?
                    //currently, if we failed to create the image, just ignore this operation.
                }
            }
        }
        pageholder->drop();
        texture->unlock();
        glyph_to_be_paged.clear();
        dirty = false;
    }

    video::ITexture* texture;
    u32 available_slots;
    u32 used_slots;
    bool dirty;

    core::array<core::vector2di> render_positions;
    core::array<core::recti> render_source_rects;

private:
    core::array<const SGUITTGlyph*> glyph_to_be_paged;
    video::IVideoDriver* driver;
    io::path name;
};

//! Class representing a TrueType font.
class CGUITTFont : public IGUITTFont
{
public:
    //! Creates a new TrueType font and returns a pointer to it. The pointer must be drop()'ed when finished.
    //! \param env The IGUIEnvironment the font loads out of.
    //! \param filename The filename of the font.
    //! \param size The size of the font glyphs in pixels. Since this is the size of the individual glyphs, the true height of the font may change depending on the characters used.
    //! \param antialias set the use_monochrome (opposite to antialias) flag
    //! \param transparency set the use_transparency flag
    //! \return Returns a pointer to a CGUITTFont.  Will return 0 if the font failed to load.
    static CGUITTFont* create(IGUIEnvironment *env, const io::path& filename, const u32& size,
                              const bool& antialias, const bool& transparency);

    //! Destructor
    virtual ~CGUITTFont();

    //! Sets the amount of glyphs to batch load.
    //! When a new glyph is loaded, the class also loads extra glyphs which surround
    //! the target glyph. For example, if batch_size is set to 50 and you try to load glyph 100,
    //! the class will load glyphs 75-125. The internal class implementation already caches the
    //! first 127 ASCII characters by default.
    //! Proceed with caution if you set this to a relatively big number (like 128 or so) if you
    //! use unicode font types, the cache miss rate is high for the current implementation (i.e.
    //! you will cache too many glyphs that you are not actually using yet.)
    //! \param batch_size The amount of glyphs to batch load. Defaults to 1.
    virtual void setBatchLoadSize(const u32& batch_size) { batch_load_size = batch_size; }

    //! Sets the maximum texture size for a page of glyphs.
    //! It creates a texture large enough for around 144 ~ near 400 glyphs.
    //! Unless you have usage for extremely large fonts (font size larger than 72 or so)
    //! the suggested size of page texture should be less than 1024x1024.
    //! And the texture size should be square of 2.
    //! \param texture_size The maximum size of the texture.
    virtual void setMaxPageTextureSize(const core::dimension2du& texture_size) { max_page_texture_size = texture_size; }

    //! Get the font size.
    virtual u32 getFontSize() const { return size; }

    //! Check the font's transparency.
    virtual bool isTransparent() const { return use_transparency; }

    //! Check if the font auto-hinting is enabled.
    virtual bool useAutoHinting() const { return use_auto_hinting; }

    //! Check if the font hinting is enabled.
    virtual bool useHinting()     const { return use_hinting; }

    //! Check if the font is monochrome.
    //! This is opposed to the "anti-alias" concept.
    virtual bool useMonochrome()  const { return use_monochrome; }

    //! Tells the font to allow transparency when rendering.
    //! Default: true.
    //! \param flag If true, the font draws using transparency.
    virtual void setTransparency(const bool& flag);

    //! Tells the font to use monochrome rendering.
    //! Default: false.
    //! \param flag If true, the font draws using a monochrome image.
    //!        If false, the font uses a grayscale image.
    virtual void setMonochrome(const bool& flag);

    //! Enables or disables font hinting.
    //! Default: Hinting and auto-hinting true.
    //! \param enable If false, font hinting is turned off. If true, font hinting is turned on.
    //! \param enable_auto_hinting If true, FreeType uses its own auto-hinting algorithm.
    //!        If false, it tries to use the algorithm specified by the font.
    virtual void setFontHinting(const bool& enable, const bool& enable_auto_hinting = true);

    //! Draws some text and clips it to the specified rectangle if wanted.
	virtual void draw(const core::stringw& text, const core::rect<s32>& position,
		video::SColor color, bool hcenter=false, bool vcenter=false,
		const core::rect<s32>* clip=0);

    //! Returns the dimension of a character produced by this font.
    //! \param ch The character you need to know the size about
    virtual core::dimension2d<u32> getCharDimension(const uchar32_t& ch) const;

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

    void forceGlyphUpdate()
    {
        for (u32 i = 0; i != Glyph_Pages.size(); ++i)
            Glyph_Pages[i]->updateTexture();
    }

    //! Get the last glyph page if there's still available slots.
    //! If not, it will return zero.
    CGUITTGlyphPage* getLastGlyphPage() const;

    //! Create a new glyph page texture.
    //! \param pixel_mode the pixel mode defined by FT_Pixel_Mode
    CGUITTGlyphPage* createGlyphPage(const FT_Pixel_Mode& pixel_mode);

    //! Get the last glyph page's index.
    u32 getLastGlyphPageIndex() const { return Glyph_Pages.size() - 1; }

    //! Create corresponding character's software image copy from the font,
    //! so you can use this data just like any ordinary video::IImage.
    //! \param ch The character you need
    virtual video::IImage* createImageFromChar(const uchar32_t& ch);

    //! This function is for debugging mostly. If the page doesn't exist it returns zero.
    //! \param page_index Simply return the texture handle of a given page index.
    virtual video::ITexture* getPageTextureByIndex(const u32& page_index) const;

    //! Add a list of scene nodes generated by putting font textures on the 3D planes.
    //! A single character forms a single plane holding the corresponding texture.
    //! \param text The text to generate scene nodes.
    //! \param smgr The corresponding scene manager.
    //! \param parent The parent to the array of text scene nodes.
    //! \param color The color of the text
    //! \param center Set the text scene node alignment
    virtual core::array<scene::ISceneNode*> addTextSceneNode
        (const wchar_t* text, scene::ISceneManager* smgr, scene::ISceneNode* parent = 0,
         const video::SColor& color = video::SColor(255, 0, 0, 0), const bool& center = false );

    virtual core::array<scene::ISceneNode*> addTextSceneNode
        (const core::ustring& text, scene::ISceneManager* smgr, scene::ISceneNode* parent = 0,
         const video::SColor& color = video::SColor(255, 0, 0, 0), const bool& center = false );

protected:
    // properties moved to protected scope instead of private.
    bool               use_monochrome;
    bool               use_transparency;
    bool               use_hinting;
    bool               use_auto_hinting;
    u32                size;
    u32                batch_load_size;
    core::dimension2du max_page_texture_size;

private:
    // Manages the FreeType library.
    static FT_Library c_library;
    static core::map<io::path, SGUITTFace*> c_faces;
    static bool c_libraryLoaded;
    static scene::IMesh* shared_plane_ptr_;
    static scene::SMesh  shared_plane_;

    CGUITTFont(IGUIEnvironment *env);
    bool load(const io::path& filename, const u32 size, const bool& antialias, const bool& transparency);
    void reset_images();
    void update_glyph_pages() const;
    void update_load_flags() {
        // Set up our loading flags.
        load_flags = FT_LOAD_DEFAULT | FT_LOAD_RENDER;
        if ( !useHinting() )     load_flags |= FT_LOAD_NO_HINTING;
        if ( !useAutoHinting() ) load_flags |= FT_LOAD_NO_AUTOHINT;
        if ( useMonochrome() )   load_flags |= FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO | FT_RENDER_MODE_MONO;
        else load_flags |= FT_LOAD_TARGET_NORMAL;
    }
    u32 getWidthFromCharacter(wchar_t c) const;
    u32 getWidthFromCharacter(uchar32_t c) const;
    u32 getHeightFromCharacter(wchar_t c) const;
    u32 getHeightFromCharacter(uchar32_t c) const;
    u32 getGlyphIndexByChar(wchar_t c) const;
    u32 getGlyphIndexByChar(uchar32_t c) const;
    core::vector2di getKerning(const wchar_t thisLetter, const wchar_t previousLetter) const;
    core::vector2di getKerning(const uchar32_t thisLetter, const uchar32_t previousLetter) const;
    core::dimension2d<u32> getDimensionUntilEndOfLine(const wchar_t* p) const;
    core::dimension2d<u32> getDimensionUntilEndOfLine(core::ustring::const_iterator it) const;

    void createSharedPlane();

    gui::IGUIEnvironment* Environment;
    video::IVideoDriver* Driver;
    io::path filename;
    FT_Face tt_face;
    FT_Size_Metrics font_metrics;
    FT_Int32 load_flags;

    mutable core::array<CGUITTGlyphPage*> Glyph_Pages;
    mutable core::array<SGUITTGlyph> Glyphs;

    s32 GlobalKerningWidth;
    s32 GlobalKerningHeight;
    core::ustring Invisible;
};

} // end namespace gui
} // end namespace irr

#endif //if _IRR_COMPILE_WITH_CGUITTFONT_

#endif // __C_GUI_TTFONT_H_INCLUDED__

