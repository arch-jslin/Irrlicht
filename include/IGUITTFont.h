
/* This file is added by arch_jslin 2008.11.02
   purpose: to supplement TrueType font class a proper interface in Irrlicht
   updated 2010.06.29 to match new IrrlichtML code base
   updated 2010.07.21 modified createImageFromChar and getPageTextureByIndex
*/

#ifndef __I_GUI_TTFONT_H_INCLUDED__
#define __I_GUI_TTFONT_H_INCLUDED__

#include <irrlicht.h>

namespace irr
{
namespace gui
{

class IGUITTFont : public IGUIFont
{
public:
	//! destructor
	virtual ~IGUITTFont() {}

	//! Calculates the width and height of a given string of text.
	//! Using core::ustring as unicode interface.
	/** \return Returns width and height of the area covered by the text if
	it would be drawn. */
	virtual core::dimension2d<u32> getDimension(const core::ustring& text) const = 0;

    //! Sets the amount of glyphs to batch load.
    //! When a new glyph is loaded, the class also loads extra glyphs which surround
    //! the target glyph. For example, if batch_size is set to 50 and you try to load glyph 100,
    //! the class will load glyphs 75-125. The internal class implementation already caches the
    //! first 127 ASCII characters by default.
    //! Proceed with caution if you set this to a relatively big number (like 128 or so) if you
    //! use unicode font types, the cache miss rate is high for the current implementation (i.e.
    //! you will cache too many glyphs that you are not actually using yet.)
    //! \param batch_size The amount of glyphs to batch load. Defaults to 1.
    virtual void setBatchLoadSize(const u32& batch_size) = 0;

    //! Sets the maximum texture size for a page of glyphs.
    //! It creates a texture large enough for around 144 ~ near 400 glyphs.
    //! Unless you have usage for extremely large fonts (font size larger than 72 or so)
    //! the suggested size of page texture should be less than 1024x1024.
    //! And the texture size should be square of 2.
    //! \param texture_size The maximum size of the texture.
    virtual void setMaxPageTextureSize(const core::dimension2du& texture_size) = 0;

    //! Get the font size.
    virtual u32 getFontSize() const = 0;

    //! Check the font's transparency.
    virtual bool isTransparent() const = 0;

    //! Check if the font auto-hinting is enabled.
    virtual bool useAutoHinting() const = 0;

    //! Check if the font hinting is enabled.
    virtual bool useHinting()     const = 0;

    //! Check if the font is monochrome.
    //! This is opposed to the "anti-alias" concept.
    virtual bool useMonochrome()  const = 0;

    //! Tells the font to allow transparency when rendering.
    //! Default: true.
    //! \param flag If true, the font draws using transparency.
    virtual void setTransparency(const bool& flag) = 0;

    //! Tells the font to use monochrome rendering.
    //! Default: false.
    //! \param flag If true, the font draws using a monochrome image.
    //!        If false, the font uses a grayscale image.
    virtual void setMonochrome(const bool& flag) = 0;

    //! Enables or disables font hinting.
    //! Default: Hinting and auto-hinting true.
    //! \param enable If false, font hinting is turned off. If true, font hinting is turned on.
    //! \param enable_auto_hinting If true, FreeType uses its own auto-hinting algorithm.
    //!        If false, it tries to use the algorithm specified by the font.
    virtual void setFontHinting(const bool& enable, const bool& enable_auto_hinting = true) = 0;

    //! Create corresponding character's software image copy from the font,
    //! so you can use this data just like any ordinary video::IImage.
    //! \param ch The character you need
    virtual video::IImage* createImageFromChar(const uchar32_t& ch) = 0;

    //! This function is for debugging mostly. If the page doesn't exist it returns zero.
    //! \param page_index index of a existing glyph page.
    virtual video::ITexture* getPageTextureByIndex(const u32& page_index) const = 0;

    //! Returns the dimension of a character produced by this font.
    //! \param ch The character you need to know the size about
    virtual core::dimension2d<u32> getCharDimension(const uchar32_t& ch) const = 0;

    //! Add a list of scene nodes generated by putting font textures on the 3D planes.
    //! A single character forms a single plane holding the corresponding texture.
    //! \param text The text to generate scene nodes.
    //! \param smgr The corresponding scene manager.
    //! \param parent The parent to the array of text scene nodes.
    //! \param color The color of the text
    //! \param center Set the text scene node alignment
    virtual core::array<scene::ISceneNode*> addTextSceneNode
        (const wchar_t* text, scene::ISceneManager* smgr, scene::ISceneNode* parent = 0,
         const video::SColor& color = video::SColor(255, 0, 0, 0), const bool& center = false ) = 0;

    virtual core::array<scene::ISceneNode*> addTextSceneNode
        (const core::ustring& text, scene::ISceneManager* smgr, scene::ISceneNode* parent = 0,
         const video::SColor& color = video::SColor(255, 0, 0, 0), const bool& center = false ) = 0;
};

} // end namespace gui
} // end namespace irr

#endif


