#include <irrlicht.h>
#include "CGUITTFont.h"

#if defined(_IRR_COMPILE_WITH_CGUITTFONT_)

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
scene::IMesh*                    CGUITTFont::shared_plane_ptr_ = 0;
scene::SMesh                     CGUITTFont::shared_plane_;

void SGUITTGlyph::load(u32 character, FT_Face face, video::IVideoDriver* driver, u32 size,
                       core::dimension2du max_texture_size, core::array<CGUITTGlyphPage*>* Glyph_Pages, bool fontHinting,
                       bool autoHinting, bool useMonochrome)
{
	if (isLoaded) return;

	// Set the size of the glyph.
	FT_Set_Pixel_Sizes(face, 0, size);

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

	// Bitmap offset information.
	offset = core::vector2di(glyph->bitmap_left, glyph->bitmap_top);

	// Determine what our texture size should be.
	// Add 1 because textures are inclusive-exclusive.
	core::dimension2du d(bits.width + 1, bits.rows + 1);
	core::dimension2du texture_size;
	//core::dimension2du texture_size(bits.width + 1, bits.rows + 1);

	// Create and load our image now.
	video::IImage* image = 0;
	switch (bits.pixel_mode)
	{
		case FT_PIXEL_MODE_MONO:
		{
			// Create a blank image and fill it with transparent pixels.
			texture_size = d.getOptimalSize(true, true);
			image = driver->createImage(video::ECF_A1R5G5B5, texture_size);
			image->fill(video::SColor(0, 255, 255, 255));

			// Load the monochrome data in.
			const u32 image_pitch = image->getPitch() / sizeof(u16);
			u16* image_data = (u16*)image->lock();
			u8* glyph_data = glyph->bitmap.buffer;
			for (s32 y = 0; y < bits.rows; ++y)
			{
				u16* row = image_data;
				for (s32 x = 0; x < bits.width; ++x)
				{
					// Monochrome bitmaps store 8 pixels per byte.  The left-most pixel is the bit 0x80.
					// So, we go through the data each bit at a time.
					if ((glyph_data[y * bits.pitch + (x / 8)] & (0x80 >> (x % 8))) != 0)
						*row = 0xFFFF;
					++row;
				}
				image_data += image_pitch;
			}
			image->unlock();
			break;
		}

		case FT_PIXEL_MODE_GRAY:
		{
			// Create our blank image.
			texture_size = d.getOptimalSize(!driver->queryFeature(video::EVDF_TEXTURE_NPOT), !driver->queryFeature(video::EVDF_TEXTURE_NSQUARE), true, 0);
			image = driver->createImage(video::ECF_A8R8G8B8, texture_size);
			image->fill(video::SColor(0, 255, 255, 255));

			// Load the grayscale data in.
			const float gray_count = static_cast<float>(bits.num_grays);
			const u32 image_pitch = image->getPitch() / sizeof(u32);
			u32* image_data = (u32*)image->lock();
			u8* glyph_data = glyph->bitmap.buffer;
			for (s32 y = 0; y < bits.rows; ++y)
			{
				u8* row = glyph_data;
				for (s32 x = 0; x < bits.width; ++x)
					image_data[y * image_pitch + x] |= static_cast<u32>(255.0f * (static_cast<float>(*row++) / gray_count)) << 24;
					//data[y * image_pitch + x] |= ((u32)(*bitsdata++) << 24);
				glyph_data += bits.pitch;
			}
			image->unlock();
			break;
		}

		default:
			// TODO: error message?
			return;
	}

	// Check to see if we have a free page.
	CGUITTGlyphPage* page = 0;
	bool make_new_page = false;
	if (Glyph_Pages->empty()) make_new_page = true;
	else
	{
		page = (*Glyph_Pages)[Glyph_Pages->size() - 1];
		if (page->available_slots == 0)
		{
			make_new_page = true;
			page = 0;
		}
	}

	// If we need to make a new page, do that now.
	if (make_new_page)
	{
		// Name of our page.
		io::path name("TTFontGlyphPage_");
		name += face->family_name;
		name += ".";
		name += face->style_name;
		name += ".";
		name += size;
		name += "_";
		name += Glyph_Pages->size();

		// Create the new page.
		page = new CGUITTGlyphPage(driver, name);

		// Determine our maximum texture size.
		// If we keep getting 0, set it to 2048x2048, as that number is pretty safe.
		if (max_texture_size.Width == 0 || max_texture_size.Height == 0)
			max_texture_size = driver->getMaxTextureSize();
		if (max_texture_size.Width == 0 || max_texture_size.Height == 0)
			max_texture_size = core::dimension2du(2048, 2048);

		// We want to try to put at least 400 glyphs on one texture.
		core::dimension2du page_texture_size;
		if (size <= 20) page_texture_size = core::dimension2du(512, 512);
		else if (size <= 51) page_texture_size = core::dimension2du(1024, 1024);
		else if (size <= 102) page_texture_size = core::dimension2du(2048, 2048);
		else if (size <= 204) page_texture_size = core::dimension2du(4096, 4096);
		else page_texture_size = core::dimension2du(8192, 8192);
		if (page_texture_size.Width > max_texture_size.Width || page_texture_size.Height > max_texture_size.Height)
			page_texture_size = max_texture_size;

		// Create the image for our page.
		switch (bits.pixel_mode)
		{
			case FT_PIXEL_MODE_MONO:
				page->image = driver->createImage(video::ECF_A1R5G5B5, page_texture_size);
				page->image->fill(video::SColor(0, 255, 255, 255));
				//page->image->fill(video::SColor(255, 0, 255, 0));
				break;

			case FT_PIXEL_MODE_GRAY:
			default:
				page->image = driver->createImage(video::ECF_A8R8G8B8, page_texture_size);
				page->image->fill(video::SColor(0, 255, 255, 255));
				//page->image->fill(video::SColor(255, 0, 255, 0));
				break;
		}

		// Determine the number of glyph slots on the page and add it to the list of pages.
		page->available_slots = (page_texture_size.Width / size) * (page_texture_size.Height / size);
		Glyph_Pages->push_back(page);
	}

	// Add the glyph to the page.
	core::vector2di page_position((page->used_slots % (page->image->getDimension().Width / size)) * size, (page->used_slots / (page->image->getDimension().Width / size)) * size);
	source_rect.UpperLeftCorner = page_position;
	source_rect.LowerRightCorner = core::vector2di(page_position.X + bits.width, page_position.Y + bits.rows);
	image->copyTo(page->image, page_position);
	image->drop();
	page->dirty = true;
	++page->used_slots;
	--page->available_slots;

	// Set our glyph as loaded.
	glyph_page = Glyph_Pages->size() - 1;
	isLoaded = true;
}

void SGUITTGlyph::unload()
{
	isLoaded = false;
}

//////////////////////

CGUITTFont* CGUITTFont::create(IGUIEnvironment *env, const io::path& filename, const u32 size)
{
	if (!c_libraryLoaded)
	{
		if (FT_Init_FreeType(&c_library))
			return 0;
		c_libraryLoaded = true;
	}

	CGUITTFont* font = new CGUITTFont(env);
	bool ret = font->load(filename, size);
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
batch_load_size(100), Environment(env), Driver(0), GlobalKerningWidth(0), GlobalKerningHeight(0)
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

bool CGUITTFont::load(const io::path& filename, const u32 size)
{
	// Some sanity checks.
	if (Environment == 0 || Driver == 0) return false;
	if (size == 0) return false;
	if (filename.size() == 0) return false;

	this->size = size;
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
			{
				delete face;
				c_faces.remove(filename);
				return false;
			}
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

	// Store font metrics.
	FT_Set_Pixel_Sizes(tt_face, size, 0);
	font_metrics = tt_face->size->metrics;

	// Allocate our glyphs.
	Glyphs.reallocate(tt_face->num_glyphs);
	Glyphs.set_used(tt_face->num_glyphs);
	for (FT_Long i = 0; i < tt_face->num_glyphs; ++i)
	{
		Glyphs[i].isLoaded = false;
	}

	// Cache the first 127 ascii characters.
	u32 old_size = batch_load_size;
	batch_load_size = 127;
	getGlyphIndexByChar((uchar32_t)0);
	batch_load_size = old_size;

	return true;
}

CGUITTFont::~CGUITTFont()
{
	// Delete the glyphs and glyph pages.
	reset_images();
	CGUITTAssistDelete::Delete(Glyphs);
	//Glyphs.clear();

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

void CGUITTFont::reset_images()
{
	// Delete the glyphs.
	for (u32 i = 0; i != Glyphs.size(); ++i)
		Glyphs[i].unload();

	// Unload the glyph pages from video memory.
	for (u32 i = 0; i != Glyph_Pages.size(); ++i)
		delete Glyph_Pages[i];
	Glyph_Pages.clear();
}

void CGUITTFont::update_glyph_pages() const
{
	for (u32 i = 0; i != Glyph_Pages.size(); ++i)
	{
		if (Glyph_Pages[i]->dirty)
			Glyph_Pages[i]->updateTexture();
	}
}

void CGUITTFont::setMonochrome(const bool flag)
{
	use_monochrome = flag;

	reset_images();
}

void CGUITTFont::setFontHinting(const bool enable, const bool enable_auto_hinting)
{
	use_hinting = enable;
	use_auto_hinting = enable_auto_hinting;

	reset_images();
}

void CGUITTFont::draw(const core::stringw& text, const core::rect<s32>& position, video::SColor color,
                      bool hcenter, bool vcenter, const core::rect<s32>* clip)
{
	if (!Driver)
		return;

	// Clear the glyph pages of their render information.
	for (u32 i = 0; i < Glyph_Pages.size(); ++i)
	{
		Glyph_Pages[i]->render_positions.clear();
		Glyph_Pages[i]->render_source_rects.clear();
	}

	// Set up some variables.
	core::dimension2d<s32> textDimension;
	core::position2d<s32> offset = position.UpperLeftCorner;
//	video::SColor colors[4];
//	for (u32 i = 0; i < 4; i++)  // not actually used, commented out by arch.jslin 2010.07.12
//		colors[i] = color;

	// Determine offset positions.
	if (hcenter || vcenter)
	{
		textDimension = getDimension(text.c_str());

		if (hcenter)
			offset.X = ((position.getWidth() - textDimension.Width) >> 1) + offset.X;

		if (vcenter)
			offset.Y = ((position.getHeight() - textDimension.Height) >> 1) + offset.Y;
	}

	// Convert to a unicode string.
	core::ustring utext(text);
	u32 utext_size = utext.size();

	// Set up our render map.
	core::map<u32, CGUITTGlyphPage*> Render_Map;

	// Start parsing characters.
	u32 n;
	uchar32_t previousChar = 0;
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
				offset.Y += font_metrics.ascender / 64;
				offset.X = position.UpperLeftCorner.X;

				if (hcenter)
					offset.X += (position.getWidth() - textDimension.Width) >> 1;
				++iter;
				continue;
			}

			// Calculate the glyph offset.
			s32 offx = Glyphs[n-1].offset.X;
			s32 offy = (font_metrics.ascender / 64) - Glyphs[n-1].offset.Y;

			// Apply kerning.
			core::vector2di k = getKerning(currentChar, previousChar);
			offset.X += k.X;
			offset.Y += k.Y;

			// Determine rendering information.
			SGUITTGlyph& glyph = Glyphs[n-1];
			CGUITTGlyphPage* const page = Glyph_Pages[glyph.glyph_page];
			page->render_positions.push_back(core::position2di(offset.X + offx, offset.Y + offy));
			page->render_source_rects.push_back(glyph.source_rect);
			Render_Map.set(glyph.glyph_page, page);
		}
		offset.X += getWidthFromCharacter(currentChar);

		previousChar = currentChar;
		++iter;
	}

	// Draw now.
	update_glyph_pages();
	core::map<u32, CGUITTGlyphPage*>::Iterator j = Render_Map.getIterator();
	while (!j.atEnd())
	{
		core::map<u32, CGUITTGlyphPage*>::Node* n = j.getNode();
		j++;
		if (n == 0) continue;

		CGUITTGlyphPage* page = n->getValue();

		if (!use_transparency) color.color |= 0xff000000;
		Driver->draw2DImageBatch(page->texture, page->render_positions, page->render_source_rects, clip, color, true);
	}
}

core::dimension2d<u32> CGUITTFont::getCharDimension(const wchar_t ch) const
{
    return core::dimension2d<u32>( getWidthFromCharacter(ch), getHeightFromCharacter(ch) );
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
	//FT_Set_Pixel_Sizes(tt_face, 0, size);

	u32 n = getGlyphIndexByChar(c);
	if (n > 0)
	{
		int w = Glyphs[n-1].advance.x / 64;
		return w;
	}
	if (c >= 0x2000)
		return (font_metrics.ascender / 64);
	else return (font_metrics.ascender / 64) / 2;
}

inline u32 CGUITTFont::getHeightFromCharacter(wchar_t c) const
{
	return getHeightFromCharacter((uchar32_t)c);
}

inline u32 CGUITTFont::getHeightFromCharacter(uchar32_t c) const
{
	// Set the size of the face.
	// This is because we cache faces and the face may have been set to a different size.
	//FT_Set_Pixel_Sizes(tt_face, 0, size);

	u32 n = getGlyphIndexByChar(c);
	if (n > 0)
	{
		// Grab the true height of the character, taking into account underhanging glyphs.
		s32 height = (font_metrics.ascender / 64) - Glyphs[n-1].offset.Y + Glyphs[n-1].source_rect.getHeight();
		return height;
	}
	if (c >= 0x2000)
		return (font_metrics.ascender / 64);
	else return (font_metrics.ascender / 64) / 2;
}

u32 CGUITTFont::getGlyphIndexByChar(wchar_t c) const
{
	return getGlyphIndexByChar((uchar32_t)c);
}

u32 CGUITTFont::getGlyphIndexByChar(uchar32_t c) const
{
	// Get the glyph.
	u32 glyph = FT_Get_Char_Index(tt_face, c);

	// Check for a valid glyph.  If it is invalid, attempt to use the replacement character.
	if (glyph == 0)
		glyph = FT_Get_Char_Index(tt_face, core::unicode::UTF_REPLACEMENT_CHARACTER);

	// If our glyph is already loaded, don't bother doing any batch loading code.
	if (glyph != 0 && Glyphs[glyph - 1].isLoaded)
		return glyph;

	// Determine our batch loading positions.
	u32 half_size = (batch_load_size / 2);
	u32 start_pos = 0;
	if (c > half_size) start_pos = c - half_size;
	u32 end_pos = start_pos + batch_load_size;

	// Load all our characters.
	do
	{
		// Get the character we are going to load.
		u32 load_glyph = FT_Get_Char_Index(tt_face, start_pos);

		// If the glyph hasn't been loaded yet, do it now.
		if (load_glyph != 0 && !Glyphs[load_glyph - 1].isLoaded)
			Glyphs[load_glyph - 1].load(load_glyph, tt_face, Driver, size, max_page_texture_size, &Glyph_Pages, use_hinting, use_auto_hinting, use_monochrome);
	}
	while (++start_pos < end_pos);

	// Update any glyph pages that were modified.
	//update_glyph_pages();

	// Return our original character.
	return glyph;
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
	FT_Set_Pixel_Sizes(tt_face, 0, size);

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

// >> added by arch_jslin 2010.06.30 ~ 2010.07.05
video::ITexture* CGUITTFont::getTextureFromChar(const wchar_t ch) const
{
    u32 n = getGlyphIndexByChar(ch);
    //return Glyphs[n-1].texture;
    return 0;
}

void CGUITTFont::createSharedPlane()
{
    /*
        2___3
        |  /|
        | / |    <-- plane mesh is like this, point 2 is (0,0), point 0 is (0, -1)
        |/  |    <-- the texture coords of point 2 is (0,0, point 0 is (0, 1)
        0---1
    */

    using namespace core;
    using namespace video;
    using namespace scene;
    S3DVertex vertices[4];
    u16 indices[6] = {0,2,3,3,1,0};
    vertices[0] = S3DVertex( vector3df(0,-1,0), vector3df(0,0,-1),
                             SColor(255,255,255,255), vector2df(0,1) );
    vertices[1] = S3DVertex( vector3df(1,-1,0), vector3df(0,0,-1),
                             SColor(255,255,255,255), vector2df(1,1) );
    vertices[2] = S3DVertex( vector3df(0, 0,0), vector3df(0,0,-1),
                             SColor(255,255,255,255), vector2df(0,0) );
    vertices[3] = S3DVertex( vector3df(1, 0,0), vector3df(0,0,-1),
                             SColor(255,255,255,255), vector2df(1,0) );

    SMeshBuffer* buf = new SMeshBuffer();
    buf->append(vertices, 4, indices, 6);

    shared_plane_.addMeshBuffer( buf );

    shared_plane_ptr_ = &shared_plane_;
    buf->drop(); //the addMeshBuffer method will grab it, so we can drop this ptr.
}

core::array<scene::ISceneNode*> CGUITTFont::addTextSceneNode
    (const wchar_t* text, scene::ISceneManager* smgr, scene::ISceneNode* parent, const video::SColor& color, bool center)
{
    using namespace core;
    using namespace video;
    using namespace scene;

    array<scene::ISceneNode*> container;

    if( !Driver || !smgr ) return container;
    if( !parent )
        parent = smgr->addEmptySceneNode(smgr->getRootSceneNode(), -1);
    //if you don't specify parent, then we add a empty node attached to the root node
    //this is generally undesirable.

    if( !shared_plane_ptr_ ) //this points to a static mesh that contains the plane
        createSharedPlane(); //if it's not initialized, we create one.

    dimension2d<s32> text_size( getDimension(text) ); //convert from unsigned to signed.
    vector3df start_point(0, 0, 0), offset;

    /** NOTICE:
        Because we are considering adding texts into 3D world, all Y axis vectors are inverted.
    **/

    //There's currently no "vertical center" concept when you apply text scene node to the 3D world.
    if( center ) {
        offset.X = start_point.X = -text_size.Width / 2.f;
        offset.Y = start_point.Y = +text_size.Height/ 2.f;
        core::stringw s;
        for(const wchar_t* temp = text; temp && *temp != '\0' && *temp != L'\r' && *temp != L'\n'; ++temp )
            s.append(*temp); //collect the next line to estimate the width
        offset.X += ( text_size.Width - getDimension(s.c_str()).Width ) >> 1;
    }

    //the default font material
    SMaterial mat;
    mat.setFlag(video::EMF_LIGHTING, true);
    mat.setFlag(video::EMF_ZWRITE_ENABLE, false);
    mat.setFlag(video::EMF_NORMALIZE_NORMALS, true);
    mat.ColorMaterial = video::ECM_NONE;
    mat.MaterialType = use_transparency ? video::EMT_TRANSPARENT_MODULATE : video::EMT_SOLID;
    mat.MaterialTypeParam = 0.01f;
    mat.DiffuseColor = color;

    wchar_t current_char = 0, previous_char = 0;
    u32 n = 0;

    array<u32> glyph_indices;

    while( *text ) {
        current_char = *text;
        bool line_break=false;
        if ( current_char == L'\r' ) { // Mac or Windows breaks
            line_break = true;
            if ( *(text + 1) == L'\n' )	// Windows line breaks.
                current_char = *(++text);
        }
        else if ( current_char == L'\n' ) { // Unix breaks
            line_break = true;
        }

        if (line_break) {
            previous_char = 0;
            offset.Y -= tt_face->size->metrics.ascender / 64;
            offset.X =  start_point.X;
            if (center) {
                core::stringw s;
                for(const wchar_t* temp = text+1; temp && *temp != '\0' && *temp != L'\r' && *temp != L'\n'; ++temp )
                    s.append(*temp); //collect the next line to estimate the width
                offset.X += ( text_size.Width - getDimension(s.c_str()).Width ) >> 1;
            }
            ++text;
        }
        else {
            n = getGlyphIndexByChar(current_char);
            if( n > 0 ) {
                glyph_indices.push_back( n );
                //NOTE:
                //In the drawing loop, first we get the glyph index, and we get glyph source rect
                //and the glyph page information. and then we have page texture dimension,
                //using the texture dimension and source rect, we can transform this data into
                //texture UV coordinates.
                //then we call update_glyph_pages() so the ITexture object in the GlyphPage is
                //updated accordingly. finally we can adjust all those vertices' UV positions so
                //they'll show up correctly. this should be quite easy to do.

                // Store glyph size and offset informations.
                SGUITTGlyph const& glyph = Glyphs[n-1];
                u32 texw = glyph.source_rect.getWidth();
                u32 texh = glyph.source_rect.getHeight();
                s32 offx = glyph.offset.X;
                s32 offy = (font_metrics.ascender / 64) - glyph.offset.Y;

                // Apply kerning.
                vector2di k = getKerning(current_char, previous_char);
                offset.X += k.X;
                offset.Y += k.Y;

                vector3df        current_pos(offset.X + offx, offset.Y - offy, 0);
                dimension2d<u32> letter_size = dimension2d<u32>( texw, texh );

                //Now we copy planes corresponding to the letter size
                IMeshManipulator* mani = smgr->getMeshManipulator();
                IMesh* meshcopy = mani->createMeshCopy( shared_plane_ptr_ );
                mani->scaleMesh( meshcopy, vector3df(letter_size.Width, letter_size.Height, 1) );

                ISceneNode* current_node = smgr->addMeshSceneNode( meshcopy, parent, -1, current_pos );
                meshcopy->drop();

                current_node->getMaterial(0) = mat;
                current_node->setAutomaticCulling(EAC_OFF);
                current_node->setIsDebugObject(true);  //so the picking won't have any effect on individual letter
                //current_node->setDebugDataVisible(EDS_BBOX); //de-comment this when debugging

                container.push_back( current_node );
            }
            offset.X += getWidthFromCharacter(current_char);
            previous_char = current_char;
            ++text;
        }
    }

    update_glyph_pages();
    //only after we update the textures can we use the glyph page textures.

    for( u32 i = 0; i < glyph_indices.size(); ++i ) {
        u32 n = glyph_indices[i];
        SGUITTGlyph const& glyph = Glyphs[n-1];
        ITexture* current_tex = Glyph_Pages[glyph.glyph_page]->texture;
        f32 page_texture_size = current_tex->getSize().Width;
        //Now we calculate the UV position according to the texture size and the source rect.
        //
        //  2___3
        //  |  /|
        //  | / |    <-- plane mesh is like this, point 2 is (0,0), point 0 is (0, -1)
        //  |/  |    <-- the texture coords of point 2 is (0,0, point 0 is (0, 1)
        //  0---1
        //
        f32 u1 = glyph.source_rect.UpperLeftCorner.X / page_texture_size;
        f32 u2 = u1 + (glyph.source_rect.getWidth()  / page_texture_size);
        f32 v1 = glyph.source_rect.UpperLeftCorner.Y / page_texture_size;
        f32 v2 = v1 + (glyph.source_rect.getHeight() / page_texture_size);

        //we can be quite sure that this is IMeshSceneNode, because we just added them in the above loop.
        IMeshSceneNode* node = static_cast<IMeshSceneNode*>(container[i]);

        S3DVertex* pv = static_cast<S3DVertex*>(node->getMesh()->getMeshBuffer(0)->getVertices());
        //pv[0].TCoords.Y = pv[1].TCoords.Y = (letter_size.Height - 1) / static_cast<f32>(letter_size.Height);
        //pv[1].TCoords.X = pv[3].TCoords.X = (letter_size.Width - 1)  / static_cast<f32>(letter_size.Width);
        pv[0].TCoords = vector2df(u1, v2);
        pv[1].TCoords = vector2df(u2, v2);
        pv[2].TCoords = vector2df(u1, v1);
        pv[3].TCoords = vector2df(u2, v1);

        container[i]->getMaterial(0).setTexture(0, current_tex);
    }

    return container;
}
// <<

} // end namespace gui
} // end namespace irr
#endif //end of _IRR_COMPILE_WITH_CGUITTFONT_
