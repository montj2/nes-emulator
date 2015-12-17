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
#include "../ui.h"

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

	void reset()
	{
		// setup mmc
		mmc::reset();
		mmc::setup();

		// setup cpu
		cpu::reset();

		// setup ppu
		ppu::reset();
		ppu::setup();
	}

	bool nextFrame()
	{
		for (;;)
		{
			if (cpu::run(-1,114))
			{
				// printf("[ ] ------ Scanline %d ------\n", ppu::currentScanline());
				if (!ppu::hsync())
				{
					// frame ends
					break;
				}
			}
			else
				return false; // program stops
		}
		return true;
	}

	void run()
	{
		for (;;)
		{
			ui::doEvents();
			if (ui::forceTerminate())
			{
				// create state dump on force termination
				cpu::dump();
				break;
			}
			if (!nextFrame())
			{
				// game stops
				break;
			}
			ui::waitForVSync();
		}
	}

	void saveState(FILE *fp)
	{
		mmc::save(fp);
		cpu::save(fp);
		ppu::save(fp);
	}

	void loadState(FILE *fp)
	{
		reset(); // necessary

		mmc::load(fp);
		cpu::load(fp);
		ppu::load(fp);
	}
}
