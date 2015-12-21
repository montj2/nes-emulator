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
		// reset mmc
		mmc::reset();
		mapper::reset();

		// reset cpu
		cpu::reset();

		// reset ppu
		ppu::reset();
	}

	bool setup()
	{
		if (!mapper::setup()) return false;
		if (!pmapper::setup()) return false;
		return true;
	}

	bool nextFrame()
	{
		for (;;)
		{
			if (cpu::run(-1, SCANLINE_CYCLES))
			{
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
			ui::limitFPS();
		}
	}

	long long frameCount()
	{
		return ppu::currentFrame();
	}

	void saveState(FILE *fp)
	{
		mmc::save(fp);
		mapper::save(fp);
		cpu::save(fp);
		ppu::save(fp);
	}

	void loadState(FILE *fp)
	{
		reset(); // necessary

		mmc::load(fp);
		mapper::load(fp);
		cpu::load(fp);
		ppu::load(fp);
	}
}
