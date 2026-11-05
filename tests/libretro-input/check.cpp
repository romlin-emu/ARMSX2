// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "InputDevices.h"
#include <cassert>
#include <cstring>

static int16_t input(unsigned port, unsigned device, unsigned index, unsigned id)
{
	if (port != 1) return 0;
	if (device == RETRO_DEVICE_JOYPAD) return id == 3;
	if (device == RETRO_DEVICE_ANALOG) return static_cast<int16_t>(100 + index * 2 + id);
	if (device == RETRO_DEVICE_MOUSE)
	{
		if (id == RETRO_DEVICE_ID_MOUSE_X) return 7;
		if (id == RETRO_DEVICE_ID_MOUSE_Y) return -4;
		if (id == RETRO_DEVICE_ID_MOUSE_WHEELUP) return 1;
		if (id == RETRO_DEVICE_ID_MOUSE_LEFT) return 1;
	}
	return 0;
}

int main()
{
	LibretroInput::InputDevices devices;
	auto initial = devices.Take();
	assert(initial[0].device == RETRO_DEVICE_JOYPAD && initial[0].HasPad());
	assert(initial[1].device == RETRO_DEVICE_NONE && !initial[1].HasPad());
	assert(!devices.Select(2, RETRO_DEVICE_MOUSE));
	assert(!devices.Select(0, 12345));
	assert(devices.Take()[0].device == RETRO_DEVICE_JOYPAD);
	assert(devices.Select(1, RETRO_DEVICE_MOUSE));
	devices.Capture(input);
	devices.Capture(input);
	auto state = devices.Take();
	assert(state[1].device == RETRO_DEVICE_MOUSE && state[1].HasPad() && state[1].HasMouse());
	assert(std::strcmp(state[1].UsbTypeName(), "hidmouse") == 0);
	assert(state[1].joypad[3] && !state[1].joypad[2]);
	assert(state[1].analog[0] == 100 && state[1].analog[3] == 103);
	assert(state[1].mouse_buttons[0] && !state[1].mouse_buttons[1]);
	assert(state[1].mouse_x == 14 && state[1].mouse_y == -8 && state[1].mouse_wheel == 2);
	state = devices.Take();
	assert(state[1].mouse_x == 0 && state[1].mouse_y == 0 && state[1].mouse_wheel == 0);
	assert(state[1].mouse_buttons[0] && state[1].joypad[3]);
	devices.Capture(input);
	assert(devices.Select(1, RETRO_DEVICE_NONE));
	state = devices.Take();
	assert(!state[1].HasPad() && !state[1].HasMouse());
	assert(std::strcmp(state[1].UsbTypeName(), "None") == 0);
	assert(!state[1].joypad[3] && !state[1].mouse_buttons[0] && state[1].analog[0] == 0 && state[1].mouse_x == 0);
	assert(devices.Select(1, RETRO_DEVICE_MOUSE));
	devices.Capture(input);
	assert(devices.Select(1, RETRO_DEVICE_JOYPAD));
	state = devices.Take();
	assert(state[1].HasPad() && !state[1].HasMouse() && !state[1].joypad[3] && !state[1].mouse_buttons[0] && state[1].mouse_x == 0);
	devices.Select(1, RETRO_DEVICE_MOUSE);
	devices.Capture(input);
	devices.Reset();
	state = devices.Take();
	assert(state[0].device == RETRO_DEVICE_JOYPAD && state[1].device == RETRO_DEVICE_NONE);
	assert(!state[1].joypad[3] && !state[1].mouse_buttons[0] && state[1].mouse_x == 0);
}
