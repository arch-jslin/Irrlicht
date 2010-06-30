#include <irrlicht.h>
#if defined(_IRR_COMPILE_WITH_CGUITTFONT_)

#include "CGUITTFont.h"

namespace irr
{
namespace gui
{

// Manages the FT_Face cache.
struct SGUITTFace : public virtual irr::IReferenceCounted
{
	~SGUITTFace()
	{
		FT_Done_Face(face);
		delete[] face_buffer;
	}

	FT_Face face;
	FT_Byte* face_buffer;
	FT_Long face_buffer_size;
};

FT_Library                       CGUITTFont::c_library;
core::map<io::path, SGUITTFace*> CGUITTFont::c_faces;
bool                             CGUITTFont::c_libraryLoaded = false;

void SGUITTGlyph::load(u32 character, FT_Face face, video::IVideoDriver* driver, u32 size, bool size_in_pixel, bool fontHinting, bool autoHinting, bool useMonochrome)
{
	isLoaded = false;

	// Set the size of the glyph.
	if (size_in_pixel)
		FT_Set_Pixel_Sizes(face, 0, size);
	else
		FT_Set_Char_Size(face, size * 64, size * 64, 0, 0);

	// Name of our glyph (used when adding the texture to the video driver.)
	io::path name("TTFontGlyph_");
	name += face->family_name;
	name += ".";
	name += face->style_name;
	name += ".";
	name += size;
	name += "_";
	name += character;

	// Set up our loading flags.
	FT_Int32 loadFlags = FT_LOAD_DEFAULT | FT_LOAD_RENDER;
	if (!fontHinting) loadFlags |= FT_LOAD_NO_HINTING;
	if (!autoHinting) loadFlags |= FT_LOAD_NO_AUTOHINT;
	if (useMonochrome) loadFlags |= FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO | FT_RENDER_MODE_MONO;
	else loadFlags |= FT_LOAD_TARGET_NORMAL;

	// Attempt to load the glyph.
	if (FT_Load_Glyph(face, character, loadFlags) != FT_Err_Ok)
		// TODO: error message?
		return;

	FT_GlyphSlot glyph = face->glyph;
	advance = glyph->advance;

	// Load the image.
	FT_Bitmap bits = glyph->bitmap;

	// Bitmap information.
	bitmap_size.UpperLeftCorner = core::vector2d<u32>(glyph->bitmap_left, glyph->bitmap_top);
	bitmap_size.LowerRightCorner = core::vector2d<u32>(glyph->bitmap_left + bits.width, glyph->bitmap_top + bits.rows);

	// Determine what our texture size should be.
	// Add 1 because textures are inclusive-exclusive.
	core::dimension2du d(bits.width + 1, bits.rows + 1);
	//texture_size = d.getOptimalSize(true, true, true, 0);
	texture_size = d.getOptimalSize(!driver->queryFeature(video::EVDF_TEXTURE_NPOT), !driver->queryFeature(video::EVDF_TEXTURE_NSQUARE), true, 0);

	// Save our texture creation flags and disable mipmaps.
	bool flg16 = driver->getTextureCreationFlag(video::ETCF_ALWAYS_16_BIT);
	bool flg32 = driver->getTextureCreationFlag(video::ETCF_ALWAYS_32_BIT);
	bool flgmip = driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
	driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

	// Create and load our image now.
	switch (bits.pixel_mode)
	{
		case FT_PIXEL_MODE_MONO:
		{
			// Force square and POT textures.  If we don't, it will scramble the font.  I don't know
			// if this is an issue with Irrlicht, or with my loading code.
			texture_size = d.getOptimalSize(true, true, true, 0);	// Do this to prevent errors.

			// Create a blank image and fill it with transparent pixels.
			image = driver->createImage(video::ECF_A1R5G5B5, texture_size);
			image->fill(video::SColor(0, 0, 0, 0));

			// Load the monochrome data in.
			const u32 image_pitch = image->getPitch() / sizeof(u16);
			u16* data = (u16*)image->lock();
			u8* bitsdata = glyph->bitmap.buffer;
			for (s32 y = 0; y < bits.rows; ++y)
			{
				u16* row = data;
				for (s32 x = 0; x < bits.width; ++x)
				{
					// Monochrome bitmaps store 8 pixels per byte.  The left-most pixel is the bit 0x80.
					// So, we go through the data each bit at a time.
					if ((bitsdata[y * bits.pitch + (x / 8)] & (0x80 >> (x % 8))) != 0)
						*row = 0xFFFF;
					++row;
				}
				data += image_pitch;
			}
			image->unlock();

			// We want to create a 16-bit texture.
			driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, true);
			driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, false);
			break;
		}

		case FT_PIXEL_MODE_GRAY:
		{
			// Create our blank image.
			image = driver->createImage(video::ECF_A8R8G8B8, texture_size);
			image->fill(video::SColor(0, 255, 255, 255));

			// Load the grayscale data in.
			const float gray_count = static_cast<float>(bits.num_grays);
			const u32 image_pitch = image->getPitch() / sizeof(u32);
			u32* data = (u32*)image->lock();
			u8* row = glyph->bitmap.buffer;
			u8* bitsdata;
			for (s32 y = 0; y < bits.rows; ++y)
			{
				bitsdata = row;
				for (s32 x = 0; x < bits.width; ++x)
					data[y * image_pitch + x] |= static_cast<u32>(255.0f * (static_cast<float>(*bitsdata++) / gray_count)) << 24;
					//data[y * image_pitch + x] |= ((u32)(*bitsdata++) << 24);
				row += bits.pitch;
			}
			image->unlock();

			// We want to create a 32-bit texture.
			driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, false);
			driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, true);
			break;
		}

		default:
			// TODO: error message?
			return;
	}

	// Create our texture.
	if (texture) driver->removeTexture(texture);
	texture = driver->addTexture(name, image);

	// Additional operations to do after we create the texture.
	switch (bits.pixel_mode)
	{
		case FT_PIXEL_MODE_GRAY:
			// If we are in software mode, we don't drop the texture,
			// as it is used for drawing translucency correctly.
			if (driver->getDriverType() != video::EDT_SOFTWARE)
			{
				image->drop();
				image = 0;
			}
			break;

		default:
			image->drop();
			image = 0;
			break;
	}

	// Restore our texture creation flags.
	driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, flgmip);
	driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, flg32);
	driver->setTextureCreationFlag(video::ETCF_ALWAYS_16_BIT, flg16);

	// Set our glyph as loaded.
	isLoaded = true;
}

void SGUITTGlyph::unload(video::IVideoDriver* driver)
{
	if (image) image->drop();
	image = 0;

	if (driver)
	{
		if (texture) driver->removeTexture(texture);
		texture = 0;
	}

	isLoaded = false;
}

//////////////////////

CGUITTFont* CGUITTFont::create(IGUIEnvironment *env, const io::path& filename, const u32 size, const bool size_in_pixel)
{
	if (!c_libraryLoaded)
	{
		if (FT_Init_FreeType(&c_library))
			return 0;
		c_libraryLoaded = true;
	}

	CGUITTFont* font = new CGUITTFont(env);
	bool ret = font->load(filename, size, size_in_pixel);
	if (!ret)
	{
		font->drop();
		return 0;
	}

	return font;
}

//////////////////////

//! constructor
CGUITTFont::CGUITTFont(IGUIEnvironment *env)
: use_monochrome(false), use_transparency(true), use_hinting(true), use_auto_hinting(true),
Environment(env), Driver(0), GlobalKerningWidth(0), GlobalKerningHeight(0)
{
	#ifdef _DEBUG
	setDebugName("CGUITTFont");
	#endif

	if (Environment)
	{
		// don't grab environment, to avoid circular references
		Driver = Environment->getVideoDriver();
	}

	if (Driver)
		Driver->grab();

	setInvisibleCharacters(L" ");

	// Glyphs isn't reference counted, so don't try to delete when we free the array.
	Glyphs.set_free_when_destroyed(false);
}

bool CGUITTFont::load(const io::path& filename, const u32 size, const bool size_in_pixel)
{
	this->size = size;
	this->size_in_pixel = size_in_pixel;
	io::IFileSystem* filesystem = Environment->getFileSystem();
	//io::path f(filename);
	//filesystem->flattenFilename(f);
	this->filename = filename;

	// Grab the face.
	SGUITTFace* face = 0;
	core::map<io::path, SGUITTFace*>::Node* node = c_faces.find(filename);
	if (node == 0)
	{
		face = new SGUITTFace();
		c_faces.set(filename, face);

		if (filesystem)
		{
			// Read in the file data.
			io::IReadFile* file = filesystem->createAndOpenFile(filename);
			face->face_buffer = new FT_Byte[file->getSize()];
			file->read(face->face_buffer, file->getSize());
			face->face_buffer_size = file->getSize();
			file->drop();

			// Create the face.
			if (FT_New_Memory_Face(c_library, face->face_buffer, face->face_buffer_size,  0, &face->face))
			{
				delete face;
				c_faces.remove(filename);
				return false;
			}
		}
		else
		{
			core::ustring converter(filename);
			if (FT_New_Face(c_library, reinterpret_cast<const char*>(converter.toUTF8_s().c_str()), 0, &face->face))
				return false;
		}
	}
	else
	{
		// Using another instance of this face.
		face = node->getValue();
		face->grab();
	}

	// Store our face.
	tt_face = face->face;

	// Allocate our glyphs.
	Glyphs.reallocate(tt_face->num_glyphs);
	Glyphs.set_used(tt_face->num_glyphs);
	for (FT_Long i = 0; i < tt_face->num_glyphs; ++i)
	{
		Glyphs[i].isLoaded = false;
		Glyphs[i].image = 0;
		Glyphs[i].texture = 0;
	}
	return true;
}

CGUITTFont::~CGUITTFont()
{
	// Unload the glyphs from video memory.
	for (u32 i = 0; i < Glyphs.size(); ++i)
		Glyphs[i].unload(Driver);
	Glyphs.clear();

	// We aren't using this face anymore.
	core::map<io::path, SGUITTFace*>::Node* n = c_faces.find(filename);
	if (n)
	{
		SGUITTFace* f = n->getValue();

		// Drop our face.  If this was the last face, the destructor will clean up.
		if (f->drop())
			c_faces.remove(filename);

		// If there are no more faces referenced by FreeType, clean up.
		if (c_faces.size() == 0)
		{
			FT_Done_FreeType(c_library);
			c_libraryLoaded = false;
		}
	}

	// Drop our driver now.
	if (Driver)
		Driver->drop();
}

void CGUITTFont::setMonochrome(const bool flag)
{
	use_monochrome = flag;

	// Unload our glyphs.
	for (u32 i = 0; i < Glyphs.size(); ++i)
		Glyphs[i].unload(Driver);
}

void CGUITTFont::setFontHinting(const bool enable, const bool enable_auto_hinting)
{
	use_hinting = enable;
	use_auto_hinting = enable_auto_hinting;

	// Unload our glyphs.
	for (u32 i = 0; i < Glyphs.size(); ++i)
		Glyphs[i].unload(Driver);
}

void CGUITTFont::draw(const core::stringw& text, const core::rect<s32>& position, video::SColor color, bool hcenter, bool vcenter, const core::rect<s32>* clip)
{
	if (!Driver)
		return;

	// Set the size of the face.
	// This is because we cache faces and the face may have been set to a different size.
	if (size_in_pixel)
		FT_Set_Pixel_Sizes(tt_face, 0, size);
	else
		FT_Set_Char_Size(tt_face, size * 64, size * 64, 0, 0);

	core::dimension2d<s32> textDimension;
	core::position2d<s32> offset = position.UpperLeftCorner;
	video::SColor colors[4];
	for (int i = 0; i < 4; i++)
		colors[i] = color;

	if (hcenter || vcenter)
	{
		textDimension = getDimension(text.c_str());

		if (hcenter)
			offset.X = ((position.getWidth() - textDimension.Width) >> 1) + offset.X;

		if (vcenter)
			offset.Y = ((position.getHeight() - textDimension.Height) >> 1) + offset.Y;
	}

	u32 n;

	uchar32_t previousChar = 0;
	core::ustring utext(text);
	core::ustring::const_iterator iter(utext);
	while (!iter.atEnd())
	{
		uchar32_t currentChar = *iter;
		n = getGlyphIndexByChar(currentChar);
		bool visible = (Invisible.findFirst(currentChar) == -1);
		if (n > 0 && visible)
		{
			bool lineBreak=false;
			if (currentChar == L'\r') // Mac or Windows breaks
			{
				lineBreak = true;
				if (*(iter + 1) == (uchar32_t)'\n')	// Windows line breaks.
					currentChar = *(++iter);
			}
			else if (currentChar == (uchar32_t)'\n') // Unix breaks
			{
				lineBreak = true;
			}

			if (lineBreak)
			{
				previousChar = 0;
				offset.Y += tt_face->size->metrics.ascender / 64;
				offset.X = position.UpperLeftCorner.X;

				if (hcenter)
					offset.X += (position.getWidth() - textDimension.Width) >> 1;
				++iter;
				continue;
			}

			// Store some useful information.
			u32 texw = Glyphs[n-1].texture_size.Width;
			u32 texh = Glyphs[n-1].texture_size.Height;
			u32 bmpw = Glyphs[n-1].bitmap_size.getWidth();
			u32 bmph = Glyphs[n-1].bitmap_size.getHeight();
			s32 offx = Glyphs[n-1].bitmap_size.UpperLeftCorner.X;
			s32 offy = (tt_face->size->metrics.ascender / 64) - Glyphs[n-1].bitmap_size.UpperLeftCorner.Y;

			// Apply kerning.
			core::vector2di k = getKerning(currentChar, previousChar);
			offset.X += k.X;
			offset.Y += k.Y;

			// Different rendering paths for different drivers.
			if (Driver->getDriverType() == video::EDT_SOFTWARE && !use_monochrome)
			{
				// Software driver doesn't do transparency correctly, so if we are using the driver,
				// draw each font pixel by pixel.
				u32 a = color.getAlpha();
				video::IImage* image = Glyphs[n-1].image;
				u8* pt = (u8*)image->lock();
				if (!use_transparency) a = 255;
				for (u32 y = 0; y < texh; ++y)
				{
					for (u32 x = 0; x < texw; ++x)
					{
						bool doDraw = !clip || clip->isPointInside(core::position2d<s32>(offset.X + x + offx, offset.Y + y + offy));

						// Get the alpha value for this pixel.
						u32 alpha = 0;
						switch (image->getColorFormat())
						{
							case video::ECF_A8R8G8B8:
								alpha = *((u32*)pt) >> 24;
								pt += 4;
								break;

							case video::ECF_A1R5G5B5:
								alpha = (*((u16*)pt) >> 15) * 255;
								pt += 2;
								break;
						}

						// If this pixel is visible, draw it.
						if (doDraw && alpha) Driver->draw2DRectangle(video::SColor((a * alpha) / 255, color.getRed(), color.getGreen(), color.getBlue()), core::rect<s32>(offset.X + x + offx, offset.Y + y + offy, offset.X + x + offx + 1, offset.Y + y + offy + 1));
					}
				}
				image->unlock();
			}
			// Normal rendering.
			else
			{
				if (!use_transparency) color.color |= 0xff000000;
				Driver->draw2DImage(Glyphs[n-1].texture, core::position2d<s32>(offset.X + offx, offset.Y + offy), core::rect<s32>(0, 0, texw-1, texh-1), clip, color, true);
			}
		}
		offset.X += getWidthFromCharacter(currentChar);

		previousChar = currentChar;
		++iter;
	}
}

core::dimension2d<u32> CGUITTFont::getDimension(const wchar_t* text) const
{
	return getDimension(core::ustring(text));
}

core::dimension2d<u32> CGUITTFont::getDimension(const core::ustring& text) const
{
	// Get the maximum font height.  Unfortunately, we have to do this hack as
	// Irrlicht will draw things wrong.  In FreeType, the font size is the
	// maximum size for a single glyph, but that glyph may hang "under" the
	// draw line, increasing the total font height to beyond the set size.
	// Irrlicht does not understand this concept when drawing fonts.  Also, I
	// add +1 to give it a 1 pixel blank border.  This makes things like
	// tooltips look nicer.
	s32 test1 = getHeightFromCharacter((uchar32_t)'g') + 1;
	s32 test2 = getHeightFromCharacter((uchar32_t)'j') + 1;
	s32 test3 = getHeightFromCharacter((uchar32_t)'_') + 1;
	s32 max_font_height = core::max_(test1, core::max_(test2, test3));

	core::dimension2d<u32> text_dimension(0, max_font_height);
	core::dimension2d<u32> line(0, max_font_height);

	uchar32_t previousChar = 0;
	core::ustring::const_iterator iter = text.begin();
	for (; !iter.atEnd(); ++iter)
	{
		uchar32_t p = *iter;
		bool lineBreak = false;
		if (p == '\r')	// Mac or Windows line breaks.
		{
			lineBreak = true;
			if (*(iter + 1) == '\n')
			{
				++iter;
				p = *iter;
			}
		}
		else if (p == '\n')	// Unix line breaks.
		{
			lineBreak = true;
		}

		// Kerning.
		core::vector2di k = getKerning(p, previousChar);
		line.Width += k.X;
		previousChar = p;

		// Check for linebreak.
		if (lineBreak)
		{
			previousChar = 0;
			text_dimension.Height += line.Height;
			if (text_dimension.Width < line.Width)
				text_dimension.Width = line.Width;
			line.Width = 0;
			line.Height = max_font_height;
			continue;
		}
		line.Width += getWidthFromCharacter(p);
	}
	if (text_dimension.Width < line.Width)
		text_dimension.Width = line.Width;

	return text_dimension;
}

inline u32 CGUITTFont::getWidthFromCharacter(wchar_t c) const
{
	return getWidthFromCharacter((uchar32_t)c);
}

inline u32 CGUITTFont::getWidthFromCharacter(uchar32_t c) const
{
	// Set the size of the face.
	// This is because we cache faces and the face may have been set to a different size.
	if (size_in_pixel)
		FT_Set_Pixel_Sizes(tt_face, 0, size);
	else
		FT_Set_Char_Size(tt_face, size * 64, size * 64, 0, 0);

	u32 n = getGlyphIndexByChar(c);
	if (n > 0)
	{
		int w = Glyphs[n-1].advance.x / 64;
		return w;
	}
	if (c >= 0x2000)
		return (tt_face->size->metrics.ascender / 64);
	else return (tt_face->size->metrics.ascender / 64) / 2;
}

inline u32 CGUITTFont::getHeightFromCharacter(wchar_t c) const
{
	return getHeightFromCharacter((uchar32_t)c);
}

inline u32 CGUITTFont::getHeightFromCharacter(uchar32_t c) const
{
	// Set the size of the face.
	// This is because we cache faces and the face may have been set to a different size.
	if (size_in_pixel)
		FT_Set_Pixel_Sizes(tt_face, 0, size);
	else
		FT_Set_Char_Size(tt_face, size * 64, size * 64, 0, 0);

	u32 n = getGlyphIndexByChar(c);
	if (n > 0)
	{
		// Grab the true height of the character, taking into account underhanging glyphs.
		s32 height = (tt_face->size->metrics.ascender / 64) - Glyphs[n-1].bitmap_size.UpperLeftCorner.Y + Glyphs[n-1].bitmap_size.getHeight();
		return height;
	}
	if (c >= 0x2000)
		return (tt_face->size->metrics.ascender / 64);
	else return (tt_face->size->metrics.ascender / 64) / 2;
}

u32 CGUITTFont::getGlyphIndexByChar(wchar_t c) const
{
	return getGlyphIndexByChar((uchar32_t)c);
}

u32 CGUITTFont::getGlyphIndexByChar(uchar32_t c) const
{
	u32 character = FT_Get_Char_Index(tt_face, c);

	// Check for a valid character.  If it is invalid, attempt to use the replacement character.
	if (character == 0)
		character = FT_Get_Char_Index(tt_face, core::unicode::UTF_REPLACEMENT_CHARACTER);

	// If the glyph hasn't been loaded yet, do it now.
	if (character != 0 && !Glyphs[character - 1].isLoaded)
		Glyphs[character - 1].load(character, tt_face, Driver, size, size_in_pixel, use_hinting, use_auto_hinting, use_monochrome);

	return character;
}

s32 CGUITTFont::getCharacterFromPos(const wchar_t* text, s32 pixel_x) const
{
	return getCharacterFromPos(core::ustring(text), pixel_x);
}

s32 CGUITTFont::getCharacterFromPos(const core::ustring& text, s32 pixel_x) const
{
	s32 x = 0;
	s32 idx = 0;

	u32 character = 0;
	uchar32_t previousChar = 0;
	core::ustring::const_iterator iter = text.begin();
	while (!iter.atEnd())
	{
		uchar32_t c = *iter;
		x += getWidthFromCharacter(c);

		// Kerning.
		core::vector2di k = getKerning(c, previousChar);
		x += k.X;

		if (x >= pixel_x)
			return character;

		previousChar = c;
		++iter;
		++character;
	}

	return -1;
}

void CGUITTFont::setKerningWidth(s32 kerning)
{
	GlobalKerningWidth = kerning;
}

void CGUITTFont::setKerningHeight(s32 kerning)
{
	GlobalKerningHeight = kerning;
}

s32 CGUITTFont::getKerningWidth(const wchar_t* thisLetter, const wchar_t* previousLetter) const
{
	if (tt_face == 0)
		return GlobalKerningWidth;
	if (thisLetter == 0 || previousLetter == 0)
		return 0;

	return getKerningWidth((uchar32_t)*thisLetter, (uchar32_t)*previousLetter);
}

s32 CGUITTFont::getKerningWidth(const uchar32_t thisLetter, const uchar32_t previousLetter) const
{
	// Return only the kerning width.
	return getKerning(thisLetter, previousLetter).X;
}

s32 CGUITTFont::getKerningHeight() const
{
	// FreeType 2 currently doesn't return any height kerning information.
	return GlobalKerningHeight;
}

core::vector2di CGUITTFont::getKerning(const wchar_t thisLetter, const wchar_t previousLetter) const
{
	return getKerning((uchar32_t)thisLetter, (uchar32_t)previousLetter);
}

core::vector2di CGUITTFont::getKerning(const uchar32_t thisLetter, const uchar32_t previousLetter) const
{
	if (tt_face == 0 || thisLetter == 0 || previousLetter == 0)
		return core::vector2di();

	// Set the size of the face.
	// This is because we cache faces and the face may have been set to a different size.
	if (size_in_pixel)
		FT_Set_Pixel_Sizes(tt_face, 0, size);
	else
		FT_Set_Char_Size(tt_face, size * 64, size * 64, 0, 0);

	core::vector2di ret(GlobalKerningWidth, GlobalKerningHeight);

	// If we don't have kerning, no point in continuing.
	if (!FT_HAS_KERNING(tt_face))
		return ret;

	// Get the kerning information.
	FT_Vector v;
	FT_Get_Kerning(tt_face, getGlyphIndexByChar(previousLetter), getGlyphIndexByChar(thisLetter), FT_KERNING_DEFAULT, &v);

	// If we have a scalable font, the return value will be in font points.
	if (FT_IS_SCALABLE(tt_face))
	{
		// Font points, so divide by 64.
		ret.X += (v.x / 64);
		ret.Y += (v.y / 64);
	}
	else
	{
		// Pixel units.
		ret.X += v.x;
		ret.Y += v.y;
	}
	return ret;
}

void CGUITTFont::setInvisibleCharacters(const wchar_t *s)
{
	core::ustring us(s);
	Invisible = us;
}

void CGUITTFont::setInvisibleCharacters(const core::ustring& s)
{
	Invisible = s;
}

//added by arch_jslin 2010.06.30
video::ITexture* CGUITTFont::getTextureFromChar(const wchar_t ch) const
{
    u32 n = getGlyphIndexByChar(ch);
    return Glyphs[n-1].texture;
}

} // end namespace gui
} // end namespace irr
#endif //end of _IRR_COMPILE_WITH_CGUITTFONT_
