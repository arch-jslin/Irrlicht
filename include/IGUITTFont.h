
/* This file is added by arch_jslin 2008.11.02
   purpose: to supplement TrueType font class a proper interface in Irrlicht
   updated 2010.06.29 to match new IrrlichtML code base
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

    //! Get the font size (default unit is point).
    virtual u32 getFontSize() const = 0;

    //! Check the unit of size (in pixel or in point).
    virtual bool isSizeInPixel() const = 0;

    //! Check the font's transparency.
    virtual bool isTransparent() const = 0;

    //! Check if the font auto-hinting is enabled.
    virtual bool useAutoHinting() const = 0;

    //NOTE: I don't know what's the difference between auto-hinting / hinting.
    //! Check if the font hinting is enabled.
    virtual bool useHinting()     const = 0;

    //! Check if the font is monochrome.
    //! This is opposed to the "anti-alias" concept.
    virtual bool useMonochrome()  const = 0;

    //! Tells the font to allow transparency when rendering.
    //! Default: true.
    //! \param flag If true, the font draws using transparency.
    virtual void setTransparency(const bool flag) = 0;

    //! Tells the font to use monochrome rendering.
    //! Default: false.
    //! \param flag If true, the font draws using a monochrome image.
    //!        If false, the font uses a grayscale image.
    virtual void setMonochrome(const bool flag) = 0;

    //! Enables or disables font hinting.
    //! Default: Hinting and auto-hinting true.
    //! \param enable If false, font hinting is turned off. If true, font hinting is turned on.
    //! \param enable_auto_hinting If true, FreeType uses its own auto-hinting algorithm.
    //!        If false, it tries to use the algorithm specified by the font.
    virtual void setFontHinting(const bool enable, const bool enable_auto_hinting = true) = 0;

    //! Get corresponding irrlicht video texture from the font,
    //! so you can use this font just like any ordinary texture.
    //! \param The character of the texture you need
    virtual video::ITexture* getTextureFromChar(const wchar_t ch) const = 0;
};

} // end namespace gui
} // end namespace irr

#endif


