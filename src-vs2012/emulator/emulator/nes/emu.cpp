#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "debug.h"
#include "rom.h"
#include "opcodes.h"
#include "mmc.h"
#include "cpu.h"
#include "ppu.h"
#include "emu.h"

namespace emu
{
	void init()
	{
		opcode::initTable();
		ppu::init();
	}

	void deinit()
	{
		rom::unload();
	}

	bool load(const _TCHAR* file)
	{
		return rom::load(file);
	}

	void setup()
	{
		// setup mmc
		mmc::reset();
		mmc::setup(rom::mapperType(), (const uint8_t*)rom::getImage(), rom::sizeOfImage());

		// setup cpu
		cpu::reset();

		// setup ppu
		ppu::reset();
	}

	bool nextFrame()
	{
		for (;;)
		{
			if (cpu::run(-1,114))
			{
				if (!ppu::hsync())
					break; // frame ends
			}
			else
				return false; // program stops
		}
		return true;
	}
}
