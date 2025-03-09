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
 *                     $Archive:: /G/wwlib/lcw.cpp                                            $* 
 *                                                                                             * 
 *                      $Author:: Neal_k                                                      $*
 *                                                                                             * 
 *                     $Modtime:: 10/04/99 10:25a                                             $*
 *                                                                                             * 
 *                    $Revision:: 4                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------* 
 * Functions:                                                                                  * 
 *   LCW_Comp -- Performes LCW compression on a block of data.                                 * 
 *   LCW_Uncomp -- Decompress an LCW encoded data block.                                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"
#include "bittype.h"
#include "lcw.h"

/***************************************************************************
 * LCW_Uncomp -- Decompress an LCW encoded data block.                     *
 *                                                                         *
 * Uncompress data to the following codes in the format b = byte, w = word *
 * n = byte code pulled from compressed data.                              *
 *                                                                         *
 *   Command code, n        |Description                                   *
 * ------------------------------------------------------------------------*
 * n=0xxxyyyy,yyyyyyyy      |short copy back y bytes and run x+3 from dest *
 * n=10xxxxxx,n1,n2,...,nx+1|med length copy the next x+1 bytes from source*
 * n=11xxxxxx,w1            |med copy from dest x+3 bytes from offset w1   *
 * n=11111111,w1,w2         |long copy from dest w1 bytes from offset w2   *
 * n=11111110,w1,b1         |long run of byte b1 for w1 bytes              *
 * n=10000000               |end of data reached                           *
 *                                                                         *
 *                                                                         *
 * INPUT:                                                                  *
 *      void * source ptr                                                  *
 *      void * destination ptr                                             *
 *      unsigned long length of uncompressed data                          *
 *                                                                         *
 *                                                                         *
 * OUTPUT:                                                                 *
 *     unsigned long # of destination bytes written                        *
 *                                                                         *
 * WARNINGS:                                                               *
 *     3rd argument is dummy. It exists to provide cross-platform          *
 *      compatibility. Note therefore that this implementation does not    *
 *      check for corrupt source data by testing the uncompressed length.  *
 *                                                                         *
 * HISTORY:                                                                *
 *    03/20/1995 IML : Created.                                            *
 *    03/09/2025 CFE : Fixed up types. Support for files bigger than 65k.  *
 *=========================================================================*/
int LCW_Uncomp(void const * source, void * dest, unsigned long )
{
	uint8 * source_ptr, * dest_ptr, * copy_ptr;
	uint8 op_code, data;
	unsigned count;
	unsigned * word_dest_ptr;
	unsigned word_data;

	/* Copy the source and destination ptrs. */
	source_ptr = (uint8*) source;
	dest_ptr   = (uint8*) dest;

	// A first byte of 0x00 identifies that offsets in this file are always relative
	// In non-relative files this should always be 0x81 for the first byte span.
	const bool is_relative = *source_ptr == 0;
	if (is_relative)
		++source_ptr;

	for (;;)
	{

		/* Read in the operation code. */
		op_code = *source_ptr++;

		if (!(op_code & 0x80))
		{

			/* Do a short copy from destination. */
			count = (op_code >> 4) + 3;
			copy_ptr = dest_ptr - ((uintptr_t) *source_ptr++ + (((uintptr_t) op_code & 0x0f) << 8));

			while (count--) *dest_ptr++ = *copy_ptr++;

		}
		else
		{

			if (!(op_code & 0x40))
			{
				if (op_code == 0x80)
				{

					/* Return # of destination bytes written. */
					return (int)(dest_ptr - (uint8*) dest);

				}
				else
				{

					/* Do a medium copy from source. */
					count = op_code & 0x3f;

					while (count--) *dest_ptr++ = *source_ptr++;
				}

			}
			else
			{

				if (op_code == 0xfe)
				{

					/* Do a long run. */
					count = *source_ptr + ((unsigned) *(source_ptr + 1) << 8);
					word_data = data = *(source_ptr + 2);
					word_data  = (word_data << 24) + (word_data << 16) + (word_data << 8) + word_data;
					source_ptr += 3;

					copy_ptr = dest_ptr + 4 - ((uintptr_t)dest_ptr & 0x3);
					count -= (copy_ptr - dest_ptr);
					while (dest_ptr < copy_ptr) *dest_ptr++ = data;

					word_dest_ptr = (unsigned*) dest_ptr;

					dest_ptr += (count & 0xfffffffc);

					while (word_dest_ptr < (unsigned*) dest_ptr) {
						*word_dest_ptr		= word_data;
						*(word_dest_ptr + 1) = word_data;
						word_dest_ptr += 2;
					}

					copy_ptr = dest_ptr + (count & 0x3);
					while (dest_ptr < copy_ptr) *dest_ptr++ = data;

				} 
				else 
				{

					if (op_code == 0xff)
					{

						/* Do a long copy from destination. */
						count = *source_ptr + ((uintptr_t) *(source_ptr + 1) << 8);
						const uintptr_t offset = *(source_ptr + 2) + ((uintptr_t) * (source_ptr + 3) << 8);
						copy_ptr = is_relative ? dest_ptr - offset : (uint8*)dest + offset;
						source_ptr += 4;

						while (count--) *dest_ptr++ = *copy_ptr++;

					}
					else
					{

						/* Do a medium copy from destination. */
						count = (op_code & 0x3f) + 3;
						const uintptr_t offset = (*source_ptr + ((uintptr_t) * (source_ptr + 1) << 8));
						copy_ptr = is_relative ? dest_ptr - offset : (uint8*)dest + offset;
						source_ptr += 2;

						while (count--) *dest_ptr++ = *copy_ptr++;
					}
				}
			}
		}
	}
}


/*********************************************************************************************** 
 * LCW_Comp -- Performes LCW compression on a block of data.                                   * 
 *                                                                                             * 
 *    This routine will compress a block of data using the LCW compression method. LCW has     * 
 *    the primary characteristic of very fast uncompression at the expense of very slow        * 
 *    compression times.                                                                       * 
 *                                                                                             * 
 * INPUT:   source   -- Pointer to the source data to compress.                                * 
 *                                                                                             * 
 *          dest     -- Pointer to the destination location to store the compressed data       * 
 *                      to.                                                                    * 
 *                                                                                             * 
 *          datasize -- The size (in bytes) of the source data to compress.                    * 
 *                                                                                             * 
 * OUTPUT:  Returns with the number of bytes of output data stored into the destination        * 
 *          buffer.                                                                            * 
 *                                                                                             * 
 * WARNINGS:   Be sure that the destination buffer is big enough. The maximum size required    * 
 *             for the destination buffer is (datasize + datasize/128).                        * 
 *                                                                                             * 
 * HISTORY:                                                                                    * 
 *   05/20/1997 JLB : Created.                                                                 * 
 *   03/08/2025 CFE : Ported ASM code to C++ and fixed issues with large stream sizes.         * 
 *=============================================================================================*/
int LCW_Comp(void const * source, void * dest, int datasize)
{
	if (!source || !dest || datasize <= 0)
		return 0;

	const uint8* source_read = static_cast<const uint8*>(source);
	const uint8* const source_start = source_read;
	const uint8* const source_end = source_start + datasize;

	uint8* const dest_start = static_cast<uint8*>(dest);
	uint8* dest_write = static_cast<uint8*>(dest);

	// Write a zero byte to the start of the stream if this file is to use relative offsets
	// We do this conditionally so decompressors without this data size modification can still work with smaller files.
	const bool is_relative = datasize >= UINT16_MAX;
	if (is_relative)
		*dest_write++ = 0x00;

	// We always start in a run length
	// Write the starting data bytes as length 1 and read/write the first byte of source data
	uint8* command_ptr = dest_write;
	*dest_write++ = 0x81;
	*dest_write++ = *source_read++;

	while (source_read < source_end)
	{
		// Number of bytes remaining to be processed in the source stream
		const size_t remaining_source_bytes = source_end - source_read;

		// Consider 4-byte RLE if at least 64-bytes are present and the last byte is the same value
		if (remaining_source_bytes > 64 && *source_read == *(source_read + 64))
		{
			// Run length is 16-bit, so the length can only be a max of uint16 max (stosw asm instruction)
			const uint8* rle_max = min(source_end, source_read + UINT16_MAX);

			// Scan the source bytes to find the length of the run
			const uint8* rle_end = source_read + 1;
			for (; rle_end < rle_max && *rle_end == *source_read; ++rle_end);

			const uint16 run_length = rle_end - source_read;

			// Write the run if it's long enough. Which is 65 bytes according to Westwood
			if (run_length >= 65)
			{
				// We can't add to the short run anymore without corrupting data. Null the pointer.
				command_ptr = nullptr;

				// Write the RLE command code, run length (16-bit) and the data byte
				*dest_write++ = 0xFE;
				*dest_write++ = run_length;
				*dest_write++ = run_length >> 8;
				*dest_write++ = *source_read;
				source_read = rle_end;
				continue;
			}
		}

		// Scan the source data for pattern matches to the current read head
		// Find the best matching run
		size_t block_length = 0;
		const uint8* offset_start = (source_read - source_start) < UINT16_MAX ? source_start : source_read - UINT16_MAX;
		const uint8* block_start = source_read;

		for (const uint8* search_head = offset_start; search_head < source_read; ++search_head)
		{
			// Scan forward until we find the current source byte value
			for (; *search_head != *source_read; ++search_head);

			// Break out if we hit the end of the search space
			if (search_head >= source_read)
				break;

			// Calculate how long the run is
			size_t match_run = 1;
			for (match_run = 1; match_run < remaining_source_bytes && source_read[match_run] == search_head[match_run]; ++match_run);

			// If this is better than our best existing run, stash the offset and size
			if (match_run >= block_length)
			{
				block_length = match_run;
				block_start = search_head;
			}
		}

		// Encode the block

		// Offset *will* fit into 16-bit as we put a bound on the search space earlier
		const uint16 relative_offset = uint16(source_read - block_start);
		const uint16 offset = is_relative ? relative_offset : block_start - source_start;

		// For small blocks, attempt to add to an existing 0x80 short copy block
		// Write a new one if there's no space in the existing one
		if (block_length <= 2)
		{
			// Append if we can
			if (command_ptr && *command_ptr < 0xBF)
			{
				++*command_ptr;
			}
			// Write a new block
			else
			{
				command_ptr = dest_write;
				*dest_write++ = 0x81;
			}

			// Write the value
			*dest_write++ = *source_read;
		}
		else
		{
			// Short run block is now invalid
			command_ptr = nullptr;

			// Advance the source read head by the block length
			source_read += block_length;

			// Short copy block if length <= 10 and the offset fits in 12 bits
			if (block_length <= 10 && relative_offset <= 0x0FFF)
			{
				//[Count, High Offset, Low Offset, Low Offset]
				//NH LL | DD DD DD ...

				// Subtract 3 from length to pack it into 3 bits.
				// We know blocks of this type are at least 3 length because 1-2 would be in the smaller block
				// Leftshift it 4 to get it into the high nibble, because the value is 0-7 this leaves the MSB at 0
				// and it won't be interpreted as an 0x80 command.
				const uint8 packed_block_length((uint8(block_length) - 3) << 4);
				*dest_write++ = packed_block_length + (uint8(relative_offset >> 8) & 0x0F);
				*dest_write++ = uint8(relative_offset);
			}
			// Medium run block if length <= 64
			else if (block_length <= 64)
			{
				const uint8 packed_block_length = uint8(block_length) - 3;

				// Store with both high command bits set.
				// CCNN NNNN
				*dest_write++ = packed_block_length | 0xC0;

				// Write the offset as a full 16-bit integer [LLHH]
				*dest_write++ = uint8(offset);
				*dest_write++ = uint8(offset >> 8);
			}
			// Long run block
			else
			{
				// Write the long run command
				*dest_write++ = 0xFF;

				// Write the count
				*dest_write++ = uint8(block_length);
				*dest_write++ = uint8(block_length >> 8);

				// Write the offset as a full 16-bit integer [LLHH]
				*dest_write++ = uint8(offset);
				*dest_write++ = uint8(offset >> 8);
			}
		}
	}
	
	// Write an empty length to signal the end of the stream;
	*dest_write++ = 0x80;

	return int(dest_write - dest_start);

	// Original ASM (with extra comments)
#if 0
	int retval = 0;
#ifdef _WINDOWS
	long inlen = 0;
	long a1stdest = 0;
	long a1stsrc = 0;
	long lenoff = 0;
	long ndest = 0;
	long count = 0;
	long matchoff = 0;
	long end_of_data =0;
#ifdef _DEBUG
	inlen = inlen;
	a1stdest = a1stdest;
	a1stsrc = a1stsrc;
	lenoff = lenoff;
	ndest = ndest;
	count = count;
	matchoff = matchoff;
	end_of_data = end_of_data;
#endif

	__asm {
		cld			// make sure all string commands are forward
		mov	edi,[dest]
		mov	esi,[source]
		mov	edx,[datasize]		// get length of data to compress

// compress data to the following codes in the format b = byte, w = word
// n = byte code pulled from compressed data
//   Bit field of n		command		description
// n=0xxxyyyy,yyyyyyyy		short run	back y bytes and run x+3
// n=10xxxxxx,n1,n2,...,nx+1	med length	copy the next x+1 bytes
// n=11xxxxxx,w1			med run		run x+3 bytes from offset w1
// n=11111111,w1,w2		long run	run w1 bytes from offset w2
// n=10000000			end		end of data reached

		mov	ebx,esi
		add	ebx,edx
		mov	[end_of_data],ebx
		mov	[inlen],1	//; set the in-length flag
		mov	[a1stdest],edi	//; save original dest offset for size calc
		mov	[a1stsrc],esi	//; save offset of first byte of data
		mov	[lenoff],edi	//; save the offset of the legth of this len
		sub	eax,eax
		mov	al,081h		//; the first byte is always a len
		stosb			//; write out a len of 1
		lodsb			//; get the byte
		stosb			//; save it
	}

loopstart:
	__asm {
		mov	[ndest],edi	//; save offset of compressed data
		mov	edi,[a1stsrc]	//; get the offset to the first byte of data
		mov	[count],1	//; set the count of run to 0
	}
searchloop:
	__asm {
		sub	eax,eax
		mov	al,[esi]	//; get the current byte of data
		cmp	al,[esi+64]
		jne	short notrunlength // If the byte 64 ahead of current read head isn't the same. Skip RLE.

		mov	ebx,edi

		mov	edi,esi // stash source read head in edi register
		mov	ecx,[end_of_data] 
		sub	ecx,edi
		repe	scasb
		dec	edi
		mov	ecx,edi // copy read head into ecx
		sub	ecx,esi // calculate how many bytes were scanned by "repe scasb"
		cmp	ecx,65 
		jb	short notlongenough // Do not encode as RLE unless at least 65 bytes of data

		mov	[inlen],0	//; clear the in-length flag
//		mov	[DWORD PTR inlen],0	//; clear the in-length flag
		mov	esi,edi
		mov	edi,[ndest]	//; get the offset of our compressed data

		mov	ah,al // Stash the run byte value into ah
		mov	al,0FEh // Store the RLE command in al
		stosb // Write al to edi
		xchg	ecx,eax // Swap the run length into eax
		stosw // Write the run length (uint16)
		mov	al,ch // Copy the byte value (was swapped to ch) back to al
		stosb // write the byte value of the run

		mov	[ndest],edi	//; save offset of compressed data
		mov	edi,ebx // Restore a1stsrc (pointer to source data start) back to edi
		jmp	searchloop // Restart the loop
	}
notlongenough:
	__asm {
		mov	edi,ebx // ensure a1stsrc (address of src data start) is in edi
	}
notrunlength:
oploop:
	__asm {
		mov	ecx,esi		//; get the address of the last byte +1
		sub	ecx,edi		//; get the total number of bytes left to comp
		jz	short searchdone

		repne	scasb		//; look for a match
		jne	short searchdone	//; if we don't find one we're done

		mov	ebx,[count]
		mov	ah,[esi+ebx-1]
		cmp	ah,[edi+ebx-2]

		jne	oploop

		mov	edx,esi		//; save this spot for the next search
		mov	ebx,edi		//; save this spot for the length calc
		dec	edi		//; back up one for compare
		mov	ecx,[end_of_data]		//; get the end of data
		sub	ecx,esi		//; sub current source for max len

		repe	cmpsb		//; see how many bytes match

		jne	short notend	//; if found mismatch then di - bx = match count

		inc	edi		//; else cx = 0 and di + 1 - bx = match count
	}
notend:
	__asm {
		mov	esi,edx		//; restore si
		mov	eax,edi		//; get the dest
		sub	eax,ebx		//; sub the start for total bytes that match
		mov	edi,ebx		//; restore dest
		cmp	eax,[count]	//; see if its better than before
		jb	searchloop	//; if not keep looking

		mov	[count],eax	//; if so keep the count
		dec	ebx		//; back it up for the actual match offset
		mov	[matchoff],ebx //; save the offset for later
		jmp	searchloop	//; loop until we searched it all
	}
searchdone:
	__asm {
		mov	ecx,[count]	//; get the count of the longest run
		mov	edi,[ndest]	//; get the offset of our compressed data
		cmp	ecx,2		//; see if its not enough run to matter
		jbe	short lenin		//; if its 0,1, or 2 its too small

		cmp	ecx,10		//; if not, see if it would fit in a short
		ja	short medrun	//; if not, see if its a medium run

		mov	eax,esi		//; if its short get the current address
		sub	eax,[matchoff] //; sub the offset of the match
		cmp	eax,0FFFh	//; if its less than 12 bits its a short
		ja	short medrun	//; if its not, its a medium
	}
//shortrun:
	__asm {
		sub	ebx,ebx
		mov	bl,cl		//; get the length (3-10)
		sub	bl,3		//; sub 3 for a 3 bit number 0-7
		shl	bl,4		//; shift it left 4
		add	ah,bl		//; add in the length for the high nibble
		xchg	ah,al		//; reverse the bytes for a word store
		jmp	short srunnxt	//; do the run fixup code
	}
medrun:
	__asm {
		cmp	ecx,64		//; see if its a short run
		ja	short longrun	//; if not, oh well at least its long

		sub	cl,3		//; back down 3 to keep it in 6 bits
		or	cl,0C0h		//; the highest bits are always on
		mov	al,cl		//; put it in al for the stosb
		stosb			//; store it
		jmp	short medrunnxt //; do the run fixup code
	}
lenin:
	__asm {
		cmp	[inlen],0	//; is it doing a length?
//		cmp	[DWORD PTR inlen],0	//; is it doing a length?
		jnz	short len	//; if so, skip code
	}
lenin1:
	__asm {
		mov	[lenoff],edi	//; save the length code offset
		mov	al,80h		//; set the length to 0
		stosb			//; save it
	}
len:
	__asm {
		mov	ebx,[lenoff]	//; get the offset of the length code
		cmp	[ebx],0BFh	//; see if its maxed out
//		cmp	[BYTE PTR ebx],0BFh	//; see if its maxed out
		je	lenin1	//; if so put out a new len code
	}
//stolen:
	__asm {
		inc	[ebx] //; inc the count code
//		inc	[BYTE PTR ebx] //; inc the count code
		lodsb			//; get the byte
		stosb			//; store it
		mov	[inlen],1	//; we are now in a length so save it
//		mov	[DWORD PTR inlen],1	//; we are now in a length so save it
		jmp	short nxt	//; do the next code
	}
longrun:
	__asm {
		mov	al,0ffh		//; its a long so set a code of FF
		stosb			//; store it

		mov	eax,[count]	//; send out the count
		stosw			//; store it
	}
medrunnxt:
	__asm {
		mov	eax,[matchoff] //; get the offset
		sub	eax,[a1stsrc]	//; make it relative tot he start of data
	}
srunnxt:
	__asm {
		stosw			//; store it
		//; this code common to all runs
		add	esi,[count]	//; add in the length of the run to the source
		mov	[inlen],0	//; set the in leght flag to false
//		mov	[DWORD PTR inlen],0	//; set the in leght flag to false
	}
nxt:
	__asm {
		cmp	esi,[end_of_data]		//; see if we did the whole pic
		jae	short outofhere		//; if so, cool! were done

		jmp	loopstart
	}
outofhere:
	__asm {
		mov	ax,080h		//; remember to send an end of data code
		stosb			//; store it
		mov	eax,edi		//; get the last compressed address
		sub	eax,[a1stdest]	//; sub the first for the compressed size
		mov	[retval],eax
	}
#endif
	return(retval);
#endif // original ASM
}

