#ifndef __C_GUI_TTFONT_H_INCLUDED__
#define __C_GUI_TTFONT_H_INCLUDED__

#if defined(_IRR_COMPILE_WITH_CGUITTFONT_)

#include <irrlicht.h>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace irr
{
namespace gui
{

//forward added by arch_jslin 2010.06.29
struct SGUITTFace;

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

//! Holds a sheet of glyphs.
class CGUITTGlyphPage
{
public:
    CGUITTGlyphPage(video::IVideoDriver* Driver, const io::path& texture_name)
        :image(0), texture(0), available_slots(0), used_slots(0), dirty(false), driver(Driver), name(texture_name) {}
    ~CGUITTGlyphPage()
    {
        //if (image)
        //    image->drop();
        if (texture)
        {
            if (driver)
                driver->removeTexture(texture);
            else texture->drop();
        }
    }

    //! Create the actual page texture,
    bool createPageTexture(const u8& pixel_mode, const core::dimension2du& texture_size)
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

    //! add the glyph index to the collection, used when updateTexture is called.
    //! this collection will be cleared after updateTexture is called.
    void pushGlyphIndexToBeDrawn(const u32& index)
    {
        glyph_to_be_drawn.push_back(index);
    }

    //! Updates the texture if dirty.
    void updateTexture(const core::array<>& )
    {
        dirty = false;
        texture->lock();

        for(u32 i = 0; i < glyph_to_be_drawn.size(); ++i) {

            // >> refactored by arch.jslin 2010.07.20
//            video::IImage* image = createGlyphImage(bits, driver);
//            if( !image )
//                // TODO: add error message?
//                return;
//
//            image->copyTo(page->image, page_position);
//            image->drop();
        }

        texture->unlock();
        glyph_to_be_drawn.clear();
    }

    //video::IImage* image; //took out by arch.jslin 2010.07.20
    video::ITexture* texture;
    u32 available_slots;
    u32 used_slots;
    bool dirty;

    core::array<core::vector2di> render_positions;
    core::array<core::recti> render_source_rects;
    //core::array<u32> glyph_to_be_drawn;

private:
    video::IVideoDriver* driver;
    io::path name;
};

//! Structure representing a single TrueType glyph.
struct SGUITTGlyph
{
    //! Preload the glyph:
    //! Preload process occurs when the program tries to cache the glyph from FT_Library,
    //! however, it simply defines the SGUITTGlyph's properties, and will create page textures
    //! if nessesary. But the actual draw to texture process should only occurs right before
    //! the call to batch drawing.
    void preload(u32 character, FT_Face face, video::IVideoDriver* driver, u32 size,
                 core::dimension2du max_texture_size, core::array<CGUITTGlyphPage*>* Glyph_Pages,
                 bool fontHinting, bool autoHinting, bool useMonochrome);

    //! Unloads the glyph.
    void unload();

// >> refactored by arch.jslin 2010.07.20
    //! create the Image object from the FT_Bitmap.
    video::IImage* createGlyphImage(video::IVideoDriver* driver);

    //! If true, the glyph has been loaded.
    bool isLoaded;

    //! The page the glyph is on.
    u32 glyph_page;

    //! The source rectangle for the glyph.
    core::recti source_rect;

    //! The offset of glyph when drawn.
    core::vector2di offset;

    // Glyph advance information.
    FT_Vector advance;

    // Glyph Bitmap record.
    FT_Bitmap bits;
};

//! Class representing a TrueType font.
class CGUITTFont : public IGUITTFont
{
public:
    //! Creates a new TrueType font and returns a pointer to it.  The pointer must be drop()'ed when finished.
    //! \param env The IGUIEnvironment the font loads out of.
    //! \param filename The filename of the font.
    //! \param size The size of the font glyphs in pixels.  Since this is the size of the individual glyphs, the true height of the font may change depending on the characters used.
    //! \return Returns a pointer to a CGUITTFont.  Will return 0 if the font failed to load.
    static CGUITTFont* create(IGUIEnvironment *env, const io::path& filename, const u32 size);

    //! Destructor
    virtual ~CGUITTFont();

    //! Sets the amount of glyphs to batch load.
    virtual void setBatchLoadSize(u32 batch_size) { batch_load_size = batch_size; }

    //! Sets the maximum texture size for a page of glyphs.
    virtual void setMaxPageTextureSize(const core::dimension2du& texture_size) { max_page_texture_size = texture_size; }

    //! Get the font size (default unit is point).
    virtual u32 getFontSize() const { return size; }

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

    //! Returns the dimension of a character produced by this font.
    virtual core::dimension2d<u32> getCharDimension(const wchar_t ch) const;

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

    //! Get corresponding irrlicht video texture from the font,
    //! so you can use this font just like any ordinary texture.
    virtual video::ITexture* getTextureFromChar(const wchar_t ch) const;

    //! Add a list of scene nodes generated by putting font textures on the 3D planes.
    virtual core::array<scene::ISceneNode*> addTextSceneNode
        (const wchar_t* text, scene::ISceneManager* smgr, scene::ISceneNode* parent = 0,
         const video::SColor& color = video::SColor(255, 0, 0, 0), bool center = false );

protected:
// properties moved to protected scope instead of public.
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
    bool load(const io::path& filename, const u32 size);
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
    CGUITTGlyphPage* getLastGlyphPage() const;
    CGUITTGlyphPage* createGlyphPage(const u32& page_index, const u8& pixel_mode, core::dimension2du max_texture_size);

    u32 getWidthFromCharacter(wchar_t c) const;
    u32 getWidthFromCharacter(uchar32_t c) const;
    u32 getHeightFromCharacter(wchar_t c) const;
    u32 getHeightFromCharacter(uchar32_t c) const;
    u32 getGlyphIndexByChar(wchar_t c) const;
    u32 getGlyphIndexByChar(uchar32_t c) const;
    core::vector2di getKerning(const wchar_t thisLetter, const wchar_t previousLetter) const;
    core::vector2di getKerning(const uchar32_t thisLetter, const uchar32_t previousLetter) const;
    core::dimension2d<u32> getDimensionUntilEndOfLine(const wchar_t* p) const;

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

