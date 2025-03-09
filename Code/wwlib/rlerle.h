/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Commando/Library/RLERLE.h                                  $*
 *                                                                                             *
 *                      $Author:: Greg_h                                                      $*
 *                                                                                             *
 *                     $Modtime:: 7/22/97 11:37a                                              $*
 *                                                                                             *
 *                    $Revision:: 1                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef RLERLE_H
#define RLERLE_H

/*
**	This class holds the RLE enabled blitter object definitions. There is a blitter object
**	type for every kind of pixel operation required of RLE shapes. These are defined as
**	templates to support the different destination pixel formats.
*/

#include	"blitter.h"
#include	<assert.h>
#include	<string.h>


/*
**	This is a helper function that will skip N pixels in the RLE compressed source. This is
**	necessary for clipping purposes. The return value represents the number of transparent
**	pixels before actual pixel data starts when the RLE uncompression is resumed.
*/
inline int Skip_Leading_Pixels(unsigned char const * & sptr, int skipper)
{
	/*
	**	Skip leading pixels as requested.
	*/
	while (skipper > 0) {
		if (*sptr++ == '\0') {
			skipper -= *sptr++;
		} else {
			skipper--;
		}
	}

	/*
	**	Return with then number of leading transparent pixels in the pixel stream
	**	after the end of the skip process. This value must be tracked since the pixel
	**	skip process may have ended in the middle of a transparent pixel run.
	*/
	return(-skipper);
}


/*
**	Blits with transparency checking and translation to destination pixel format.
*/
template<class T>
class RLEBlitTransXlat : public RLEBlitter {
	public:
		RLEBlitTransXlat(T const * translator) : TranslateTable(translator) {assert(TranslateTable != NULL);}

		virtual void Blit(void * dest, void const * source, int length, int leadskip=0) const
		{
			unsigned char const * sptr = (unsigned char const *)source;
			T * dptr = (T *)dest;

			/*
			**	Skip any leading pixels as requested.
			*/
			if (leadskip > 0) {
				int transcount = Skip_Leading_Pixels(sptr, leadskip);
				dptr += transcount;
				length -= transcount;
			}

			/*
			**	Uncompress and store the pixel stream until the length has been
			**	exhausted.
			*/
			while (length > 0) {
				unsigned char value = *sptr++;
				if (value == '\0') {
					value = *sptr++;
					length -= value;
					dptr += value;
				} else {
					*dptr++ = TranslateTable[value];
					length -= 1;
				}
			}
		}

	private:
		T const * TranslateTable;
};


/*
**	This blits RLE compressed pixels by first remapping through a 256 byte table and then
**	translating the pixel to screen format.
*/
template<class T>
class RLEBlitTransRemapXlat : public RLEBlitter {
	public:
		RLEBlitTransRemapXlat(unsigned char const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {assert(TranslateTable != NULL);assert(RemapTable != NULL);}

		virtual void Blit(void * dest, void const * source, int length, int leadskip=0) const
		{
			unsigned char const * sptr = (unsigned char const *)source;
			T * dptr = (T *)dest;

			/*
			**	Skip any leading pixels as requested.
			*/
			if (leadskip > 0) {
				int transcount = Skip_Leading_Pixels(sptr, leadskip);
				dptr += transcount;
				length -= transcount;
			}

			/*
			**	Uncompress and store the pixel stream until the length has been
			**	exhausted.
			*/
			while (length > 0) {
				unsigned char value = *sptr++;
				if (value == '\0') {
					value = *sptr++;
					length -= value;
					dptr += value;
				} else {
					*dptr++ = TranslateTable[RemapTable[value]];
					length -= 1;
				}
			}
		}

	private:
		unsigned char const * RemapTable;
		T const * TranslateTable;
};


/*
**	This blits RLE compressed pixels by first remapping through a 256 byte table and then
**	translating the pixel to screen format. The remapping table is doubly indirected so that
**	it is possible to change the remapping table pointer without creating a separate blitter
**	object.
*/
template<class T>
class RLEBlitTransZRemapXlat : public RLEBlitter {
	public:
		RLEBlitTransZRemapXlat(unsigned char const * const * remapper, T const * translator) : RemapTable(remapper), TranslateTable(translator) {assert(TranslateTable != NULL);assert(RemapTable != NULL);}

		virtual void Blit(void * dest, void const * source, int length, int leadskip=0) const
		{
			unsigned char const * sptr = (unsigned char const *)source;
			unsigned char const * remapper = *RemapTable;
			T * dptr = (T *)dest;

			/*
			**	Skip any leading pixels as requested.
			*/
			if (leadskip > 0) {
				int transcount = Skip_Leading_Pixels(sptr, leadskip);
				dptr += transcount;
				length -= transcount;
			}

			/*
			**	Uncompress and store the pixel stream until the length has been
			**	exhausted.
			*/
			while (length > 0) {
				unsigned char value = *sptr++;
				if (value == '\0') {
					value = *sptr++;
					length -= value;
					dptr += value;
				} else {
					*dptr++ = TranslateTable[remapper[value]];
					length -= 1;
				}
			}
		}

	private:
		unsigned char const * const * RemapTable;
		T const * TranslateTable;
};


/*
**	This will remap the destination pixels but under the control of the source pixels.
**	Where the source pixel is not transparent, the dest pixel is remapped. This algorithm
**	really only applies to lowcolor display.
*/
template<class T>
class RLEBlitTransRemapDest : public RLEBlitter {
	public:
		RLEBlitTransRemapDest(T const * remap) : RemapTable(remap) {}

		virtual void Blit(void * dest, void const * source, int length, int leadskip=0) const
		{
			unsigned char const * sptr = (unsigned char const *)source;
			T * dptr = (T *)dest;

			/*
			**	Skip any leading pixels as requested.
			*/
			if (leadskip > 0) {
				int transcount = Skip_Leading_Pixels(sptr, leadskip);
				dptr += transcount;
				length -= transcount;
			}

			/*
			**	Uncompress and store the pixel stream until the length has been
			**	exhausted.
			*/
			while (length > 0) {
				unsigned char value = *sptr++;
				if (value == '\0') {
					value = *sptr++;
					length -= value;
					dptr += value;
				} else {
					*dptr = RemapTable[*dptr];
					length -= 1;
					dptr++;
				}
			}
		}

	private:
		T const * RemapTable;
};


/*
**	Algorithmic darkening of hicolor pixels controlled by the source pixels. The source
**	pixels are examined only to determine if the destination pixel should be darkened.
**	If the source pixel is transparent, then the dest pixel is skipped. The darkening
**	algorithm works only for hicolor pixels.
*/
template<class T>
class RLEBlitTransDarken : public RLEBlitter {
	public:
		RLEBlitTransDarken(T mask) : Mask(mask) {}

		virtual void Blit(void * dest, void const * source, int length, int leadskip=0) const
		{
			unsigned char const * sptr = (unsigned char const *)source;
			T * dptr = (T *)dest;

			/*
			**	Skip any leading pixels as requested.
			*/
			if (leadskip > 0) {
				int transcount = Skip_Leading_Pixels(sptr, leadskip);
				dptr += transcount;
				length -= transcount;
			}

			/*
			**	Uncompress and store the pixel stream until the length has been
			**	exhausted.
			*/
			while (length > 0) {
				unsigned char value = *sptr++;
				if (value == '\0') {
					value = *sptr++;
					length -= value;
					dptr += value;
				} else {
					*dptr = (T)((*dptr >> 1) & Mask);
					length -= 1;
					dptr++;
				}
			}
		}

	private:
		T Mask;
};


/*
**	This blitter performs 50% translucency as it draws. It is commonly used for animation
**	effects and other stealth like images. It only works with hicolor pixels but is a good
**	candidate for optimization.
*/
template<class T>
class RLEBlitTransLucent50 : public RLEBlitter {
	public:
		RLEBlitTransLucent50(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void * dest, void const * source, int length, int leadskip=0) const
		{
			unsigned char const * sptr = (unsigned char const *)source;
			T * dptr = (T *)dest;

			/*
			**	Skip any leading pixels as requested.
			*/
			if (leadskip > 0) {
				int transcount = Skip_Leading_Pixels(sptr, leadskip);
				dptr += transcount;
				length -= transcount;
			}

			/*
			**	Uncompress and store the pixel stream until the length has been
			**	exhausted.
			*/
			while (length > 0) {
				unsigned char value = *sptr++;
				if (value == '\0') {
					value = *sptr++;
					length -= value;
					dptr += value;
				} else {
					*dptr = (T)((((*dptr) >> 1) & Mask) + ((TranslateTable[value] >> 1) & Mask));
					length -= 1;
					dptr++;
				}
			}
		}

	private:
		T const * TranslateTable;
		T Mask;
};


/*
**	This blitter performs 25% translucency as it draws. This effect is less than spectacular,
**	but there are some uses for it. It only works with hicolor pixels.
*/
template<class T>
class RLEBlitTransLucent25 : public RLEBlitter {
	public:
		RLEBlitTransLucent25(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void * dest, void const * source, int length, int leadskip=0) const
		{
			unsigned char const * sptr = (unsigned char const *)source;
			T * dptr = (T *)dest;

			/*
			**	Skip any leading pixels as requested.
			*/
			if (leadskip > 0) {
				int transcount = Skip_Leading_Pixels(sptr, leadskip);
				dptr += transcount;
				length -= transcount;
			}

			/*
			**	Uncompress and store the pixel stream until the length has been
			**	exhausted.
			*/
			while (length > 0) {
				unsigned char value = *sptr++;
				if (value == '\0') {
					value = *sptr++;
					length -= value;
					dptr += value;
				} else {
					T qsource = (T)(((TranslateTable[value] >> 2) & Mask));
					T qdest = (T)(((*dptr) >> 2) & Mask);
					*dptr++ = (T)(qdest + qsource + qsource + qsource);
					length -= 1;
				}
			}
		}

	private:
		T const * TranslateTable;
		T Mask;
};


/*
**	This blitter performs 75% translucency as it draws. This is quite useful for explosions and
**	other gas animation effects. It only works with hicolor pixels and is a good candidate
**	for optimization.
*/
template<class T>
class RLEBlitTransLucent75 : public RLEBlitter {
	public:
		RLEBlitTransLucent75(T const * translator, T mask) : TranslateTable(translator), Mask(mask) {}

		virtual void Blit(void * dest, void const * source, int length, int leadskip=0) const
		{
			unsigned char const * sptr = (unsigned char const *)source;
			T * dptr = (T *)dest;

			/*
			**	Skip any leading pixels as requested.
			*/
			if (leadskip > 0) {
				int transcount = Skip_Leading_Pixels(sptr, leadskip);
				dptr += transcount;
				length -= transcount;
			}

			/*
			**	Uncompress and store the pixel stream until the length has been
			**	exhausted.
			*/
			while (length > 0) {
				unsigned char value = *sptr++;
				if (value == '\0') {
					value = *sptr++;
					length -= value;
					dptr += value;
				} else {
					T qsource = (T)(((TranslateTable[value] >> 2) & Mask));
					T qdest = (T)(((*dptr) >> 2) & Mask);
					*dptr++ = (T)(qdest + qdest + qdest + qsource);
					length -= 1;
				}
			}
		}

	private:
		T const * TranslateTable;
		T Mask;
};


#if defined(_MSC_VER)
void RLEBlitTransZRemapXlat<unsigned short>::Blit(void * dest, void const * source, int len, int leadskip) const
{
	unsigned char const * remapper = *RemapTable;
	unsigned short const * transtable = TranslateTable;

	const unsigned char* src_bytes = static_cast<const unsigned char*>(source);
	unsigned short* dest_shorts = static_cast<unsigned short*>(dest);

	while (leadskip > 0)
	{
		--leadskip;

		unsigned char current_byte = *src_bytes;
		++src_bytes;

		// If this is a non-zero byte then continue
		if (current_byte)
			continue;

		current_byte = *src_bytes;
		++src_bytes;

		// Subtract the read byte value from the leadskip
		leadskip -= current_byte;

		// Add one back into leadskip
		++leadskip;
	}

	// Skip remaining transparent pixels for the lead
	leadskip = -leadskip;
	len -= leadskip;
	dest_shorts += leadskip;

	while (len > 0)
	{
		unsigned char current_byte = *src_bytes;
		++src_bytes;

		// Transparent. Skip the span
		if (current_byte == 0)
		{
			current_byte = *src_bytes;
			++src_bytes;
			dest_shorts += current_byte;
			len -= current_byte;
		}
		// Non-transparent. Copy the remapped and translated value.
		else
		{
			*dest_shorts = transtable[remapper[current_byte]];
			++dest_shorts;
			--len;
		}
	}

	// Original assembly code
#if 0
	/*
	**	Set up the working registers for the blit operation.
	*/
	__asm {
		mov	ecx,[len] // Load length into ecx register
		mov	edi,[dest] // Load dest pointer into edi register
		mov	esi,[source] // Load source pointer into esi register
		mov	ebx,[remapper] // Load remapper table into ebx register
		mov	edx,[leadskip] // Load lead skip amount into edx register
		xor	eax,eax // zero eax register (includes al)
	}

	/*
	**	Skip leading pixels by analyzing the RLE data until the entire
	**	requested skip pixel count has been processed. This could result in
	**	unprocessed transparent pixels if it ended up in the middle of
	**	a transparent pixel run. This is handled in the next block.
	*/
moreskip:
	__asm {
		test	edx,edx // End if edx (leadskip) is zero
		jle	nomoreskip
		dec	edx // Decrement the leadskip count
		lodsb // Load source byte into al (eax low byte)
		test	al,al // Return to start if loaded byte is non-zero
		jnz	moreskip
		lodsb // Load another source byte into al (eax low byte)
		sub	edx,eax // Subtract the loaded byte from the lead skip
		inc	edx // Increment the leadskip
		jmp	moreskip // Repeat
	}
nomoreskip:

	/*
	**	Handle any left over transparent pixels that would be part of
	**	a transparent pixel run that occurs at the end of the leading
	**	pixel skip process.
	*/
	__asm {
		neg	edx
		sub	ecx,edx			// Account for any left over transparent pixels
		lea	edi,[edi+edx*2]
		mov	edx,[transtable]
	}

	/*
	**	Output the pixel data to the destination.
	*/
moredata:
	__asm {
		xor	eax,eax // Zero eax
		or	ecx,ecx // End if length <= 0
		jle	fini
		lodsb // Load a byte from source into al
		test	al,al
		jz	transparent // if al is zero jump to transparent
		mov	al,[ebx+eax] // Index into remap table and load into al
		mov	ax,[edx+eax*2] // Index into translation table and load int ax (16-bit)
		dec	ecx // Decrement length
		stosw // Stores the 16-bit value in AX into the destination buffer
		jmp	moredata // Repeat
	}

	/*
	**	A transparent pixel run just causes the destination pointer
	**	and length count to be adjusted by the length of the run.
	*/
transparent:
	__asm {
		lodsb // Load a single byte from the input into al
		lea	edi,[edi+eax*2] // increment destination by that number of shorts
		sub	ecx,eax // subtract that number from length
		jmp	moredata // read more
	}

fini:;
#endif 
}


void RLEBlitTransRemapXlat<unsigned short>::Blit(void * dest, void const * source, int len, int leadskip) const
{
	// Seems to be identical to RLEBlitTransZRemapXlat except RemapTable is fixed

	unsigned char const * remapper = RemapTable;
	unsigned short const * transtable = TranslateTable;

	const unsigned char* src_bytes = static_cast<const unsigned char*>(source);
	unsigned short* dest_shorts = static_cast<unsigned short*>(dest);

	while (leadskip > 0)
	{
		--leadskip;

		unsigned char current_byte = *src_bytes;
		++src_bytes;

		// If this is a non-zero byte then continue
		if (current_byte)
			continue;

		current_byte = *src_bytes;
		++src_bytes;

		// Subtract the read byte value from the leadskip
		leadskip -= current_byte;

		// Add one back into leadskip
		++leadskip;
	}

	// Skip remaining transparent pixels for the lead
	leadskip = -leadskip;
	len -= leadskip;
	dest_shorts += leadskip;

	while (len > 0)
	{
		unsigned char current_byte = *src_bytes;
		++src_bytes;

		// Transparent. Skip the span
		if (current_byte == 0)
		{
			current_byte = *src_bytes;
			++src_bytes;
			dest_shorts += current_byte;
			len -= current_byte;
		}
		// Non-transparent. Copy the remapped and translated value.
		else
		{
			*dest_shorts = transtable[remapper[current_byte]];
			++dest_shorts;
			--len;
		}
	}

	// Original asm
#if 0
	/*
	**	Set up the working registers for the blit operation.
	*/
	__asm {
		mov	ecx,[len]
		mov	edi,[dest]
		mov	esi,[source]
		mov	ebx,[remapper]
		mov	edx,[leadskip]
		xor	eax,eax
	}

	/*
	**	Skip leading pixels by analyzing the RLE data until the entire
	**	requested skip pixel count has been processed. This could result in
	**	unprocessed transparent pixels if it ended up in the middle of
	**	a transparent pixel run. This is handled in the next block.
	*/
moreskip:
	__asm {
		test	edx,edx 
		jle	nomoreskip // if leadskip <= 0 end
		dec	edx // decrement leadskip
		lodsb // load a single byte from source into al register
		test	al,al
		jnz	moreskip // Skip if non-zero
		lodsb // Load a single byte from source into al register (lower part of eax)
		sub	edx,eax  // subtract that byte value from the leadskip
		inc	edx // add one to lead count
		jmp	moreskip // continue
	}
nomoreskip:

	/*
	**	Handle any left over transparent pixels that would be part of
	**	a transparent pixel run that occurs at the end of the leading
	**	pixel skip process.
	*/
	__asm {
		neg	edx
		sub	ecx,edx			// Account for any left over transparent pixels
		lea	edi,[edi+edx*2]
		mov	edx,[transtable]
	}

	/*
	**	Output the pixel data to the destination.
	*/
moredata:
	__asm {
		xor	eax,eax // zero the eax register
		or	ecx,ecx
		jle	fini // end when len <= 0
		lodsb // load a single byte from source into al
		test	al,al
		jz	transparent // if the al is transparent then handle it
		mov	al,[ebx+eax] // index into the remap table with the loaded byte value and store in al
		mov	ax,[edx+eax*2] // index into the translation table with the remapped value and store in ax (16-bit)
		dec	ecx // decrement len
		stosw // store ax into the destination buffer
		jmp	moredata
	}

	/*
	**	A transparent pixel run just causes the destination pointer
	**	and length count to be adjusted by the length of the run.
	*/
transparent:
	__asm {
		lodsb
		lea	edi,[edi+eax*2]
		sub	ecx,eax
		jmp	moredata
	}

fini:;
#endif
}


void RLEBlitTransXlat<unsigned short>::Blit(void * dest, void const * source, int len, int leadskip) const
{
	// Seems to be identical to RLEBlitTransZRemapXlat except there's no remap table

	unsigned short const * transtable = TranslateTable;

	const unsigned char* src_bytes = static_cast<const unsigned char*>(source);
	unsigned short* dest_shorts = static_cast<unsigned short*>(dest);

	while (leadskip > 0)
	{
		--leadskip;

		unsigned char current_byte = *src_bytes;
		++src_bytes;

		// If this is a non-zero byte then continue
		if (current_byte)
			continue;

		current_byte = *src_bytes;
		++src_bytes;

		// Subtract the read byte value from the leadskip
		leadskip -= current_byte;

		// Add one back into leadskip
		++leadskip;
	}

	// Skip remaining transparent pixels for the lead
	leadskip = -leadskip;
	len -= leadskip;
	dest_shorts += leadskip;

	while (len > 0)
	{
		unsigned char current_byte = *src_bytes;
		++src_bytes;

		// Transparent. Skip the span
		if (current_byte == 0)
		{
			current_byte = *src_bytes;
			++src_bytes;
			dest_shorts += current_byte;
			len -= current_byte;
		}
		// Non-transparent. Copy the translated value.
		else
		{
			*dest_shorts = transtable[current_byte];
			++dest_shorts;
			--len;
		}
	}

	// Original asm
#if 0

	/*
	**	Set up the working registers for the blit operation.
	*/
	__asm {
		mov	ecx,[len]
		mov	edi,[dest]
		mov	esi,[source]
		mov	ebx,[transtable]
		mov	edx,[leadskip]
		xor	eax,eax
	}

	/*
	**	Skip leading pixels by analyzing the RLE data until the entire
	**	requested skip pixel count has been processed. This could result in
	**	unprocessed transparent pixels if it ended up in the middle of
	**	a transparent pixel run. This is handled in the next block.
	*/
moreskip:
	__asm {
		test	edx,edx
		jle	nomoreskip // end if leadskip <= 0
		dec	edx // decrement leadskip
		lodsb // load a single byte from source into al register
		test	al,al
		jnz	moreskip // If the byte isn't 0 skip
		lodsb // load a single byte from source into al register (lower eax register)
		sub	edx,eax // subtract from lead skip
		inc	edx // add one to lead skip
		jmp	moreskip // continue
	}
nomoreskip:

	/*
	**	Handle any left over transparent pixels that would be part of
	**	a transparent pixel run that occurs at the end of the leading
	**	pixel skip process.
	*/
	__asm {
		neg	edx
		sub	ecx,edx			// Account for any left over transparent pixels
		lea	edi,[edi+edx*2]
	}

	/*
	**	Output the pixel data to the destination.
	*/
moredata:
	__asm {
		xor	eax,eax
		or	ecx,ecx
		jle	fini
		lodsb
		test	al,al
		jz	transparent
		mov	ax,[ebx+eax*2]
		dec	ecx
		stosw
		jmp	moredata
	}

	/*
	**	A transparent pixel run just causes the destination pointer
	**	and length count to be adjusted by the length of the run.
	*/
transparent:
	__asm {
		lodsb
		lea	edi,[edi+eax*2]
		sub	ecx,eax
		jmp	moredata
	}

fini:;
#endif 
}

#endif


#endif
