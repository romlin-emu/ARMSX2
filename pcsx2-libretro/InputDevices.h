// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "libretro.h"

#include <array>
#include <cstdint>
#include <limits>
#include <mutex>

namespace LibretroInput
{
constexpr unsigned NUM_PORTS = 2;

struct PortSnapshot
{
	unsigned device = RETRO_DEVICE_NONE;
	std::array<bool, 16> joypad{};
	// Left X/Y followed by right X/Y.
	std::array<int16_t, 4> analog{};
	// Left, right, middle.
	std::array<bool, 3> mouse_buttons{};
	int64_t mouse_x = 0;
	int64_t mouse_y = 0;
	int64_t mouse_wheel = 0;

	bool HasPad() const { return device == RETRO_DEVICE_JOYPAD || device == RETRO_DEVICE_MOUSE; }
	bool HasMouse() const { return device == RETRO_DEVICE_MOUSE; }
	const char* UsbTypeName() const { return HasMouse() ? "hidmouse" : "None"; }
};

using Snapshot = std::array<PortSnapshot, NUM_PORTS>;

class InputDevices
{
public:
	InputDevices() { m_ports[0].device = RETRO_DEVICE_JOYPAD; }

	void Reset()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_ports = {};
		m_ports[0].device = RETRO_DEVICE_JOYPAD;
	}

	bool Select(unsigned port, unsigned device)
	{
		if (port >= NUM_PORTS || (device != RETRO_DEVICE_NONE && device != RETRO_DEVICE_JOYPAD && device != RETRO_DEVICE_MOUSE))
			return false;
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_ports[port].device != device)
		{
			m_ports[port] = {};
			m_ports[port].device = device;
		}
		return true;
	}

	void Capture(retro_input_state_t input)
	{
		if (!input)
			return;
		std::lock_guard<std::mutex> lock(m_mutex);
		for (unsigned port = 0; port < NUM_PORTS; port++)
		{
			auto& state = m_ports[port];
			if (state.HasPad())
			{
				for (unsigned id = 0; id < state.joypad.size(); id++)
					state.joypad[id] = input(port, RETRO_DEVICE_JOYPAD, 0, id) != 0;
				for (unsigned id = 0; id < state.analog.size(); id++)
					state.analog[id] = input(port, RETRO_DEVICE_ANALOG, id / 2, id % 2);
			}
			if (state.HasMouse())
			{
				constexpr unsigned buttons[] = {RETRO_DEVICE_ID_MOUSE_LEFT, RETRO_DEVICE_ID_MOUSE_RIGHT, RETRO_DEVICE_ID_MOUSE_MIDDLE};
				for (unsigned i = 0; i < state.mouse_buttons.size(); i++)
					state.mouse_buttons[i] = input(port, RETRO_DEVICE_MOUSE, 0, buttons[i]) != 0;
				AddMotion(state.mouse_x, input(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X));
				AddMotion(state.mouse_y, input(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y));
				AddMotion(state.mouse_wheel, static_cast<int>(input(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP)) - input(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN));
			}
		}
	}

	Snapshot Take()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		Snapshot result = m_ports;
		for (auto& port : m_ports)
			port.mouse_x = port.mouse_y = port.mouse_wheel = 0;
		return result;
	}

private:
	static void AddMotion(int64_t& value, int delta)
	{
		if (delta > 0 && value > std::numeric_limits<int64_t>::max() - delta)
			value = std::numeric_limits<int64_t>::max();
		else if (delta < 0 && value < std::numeric_limits<int64_t>::min() - delta)
			value = std::numeric_limits<int64_t>::min();
		else
			value += delta;
	}
	std::mutex m_mutex;
	Snapshot m_ports{};
};
}
