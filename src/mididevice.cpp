//
// mididevice.cpp
//
// MiniDexed - Dexed FM synthesizer for bare metal Raspberry Pi
// Copyright (C) 2022  The MiniDexed Team
//
// Original author of this class:
//	R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "mididevice.h"
#include "minidexed.h"
#include "config.h"
#include <stdio.h>
#include <assert.h>

#define MIDI_NOTE_OFF		0b1000
#define MIDI_NOTE_ON		0b1001
#define MIDI_AFTERTOUCH		0b1010			// TODO
#define MIDI_CONTROL_CHANGE	0b1011
	#define MIDI_CC_BANK_SELECT_MSB		0	// TODO
	#define MIDI_CC_BANK_SELECT_LSB		32
#define MIDI_PROGRAM_CHANGE	0b1100
#define MIDI_PITCH_BEND		0b1110

#define MIDI_TIMING_CLOCK	0xF8
#define MIDI_ACTIVE_SENSING	0xFE

CMIDIDevice::CMIDIDevice (CMiniDexed *pSynthesizer, CConfig *pConfig)
:	m_pSynthesizer (pSynthesizer),
	m_pConfig (pConfig)
{
}

CMIDIDevice::~CMIDIDevice (void)
{
	m_pSynthesizer = 0;
}

void CMIDIDevice::MIDIMessageHandler (const u8 *pMessage, size_t nLength, unsigned nCable)
{
	assert (m_pSynthesizer != 0);

	// The packet contents are just normal MIDI data - see
	// https://www.midi.org/specifications/item/table-1-summary-of-midi-message

	if (m_pConfig->GetMIDIDumpEnabled ())
	{
		switch (nLength)
		{
		case 1:
			if (   pMessage[0] != MIDI_TIMING_CLOCK
			    && pMessage[0] != MIDI_ACTIVE_SENSING)
			{
				printf ("MIDI %u: %02X\n", nCable, (unsigned) pMessage[0]);
			}
			break;

		case 2:
			printf ("MIDI %u: %02X %02X\n", nCable,
				(unsigned) pMessage[0], (unsigned) pMessage[1]);
			break;

		case 3:
			printf ("MIDI %u: %02X %02X %02X\n", nCable,
				(unsigned) pMessage[0], (unsigned) pMessage[1],
				(unsigned) pMessage[2]);
			break;
		}
	}

	if (nLength < 2)
	{
		return;
	}

	u8 ucStatus    = pMessage[0];
	// TODO: u8 ucChannel   = ucStatus & 0x0F;
	u8 ucType      = ucStatus >> 4;
	u8 ucKeyNumber = pMessage[1];
	u8 ucVelocity  = pMessage[2];

	switch (ucType)
	{
	case MIDI_NOTE_ON:
		if (nLength < 3)
		{
			break;
		}

		if (ucVelocity > 0)
		{
			if (ucVelocity <= 127)
			{
				m_pSynthesizer->keydown (ucKeyNumber, ucVelocity);
			}
		}
		else
		{
			m_pSynthesizer->keyup (ucKeyNumber);
		}
		break;

	case MIDI_NOTE_OFF:
		if (nLength < 3)
		{
			break;
		}

		m_pSynthesizer->keyup (ucKeyNumber);
		break;

	case MIDI_CONTROL_CHANGE:
		if (nLength < 3)
		{
			break;
		}

		switch (pMessage[1])
		{
		case MIDI_CC_BANK_SELECT_LSB:
			m_pSynthesizer->BankSelectLSB (pMessage[2]);
			break;
		}
		break;

	case MIDI_PROGRAM_CHANGE:
		m_pSynthesizer->ProgramChange (pMessage[1]);
		break;

	case MIDI_PITCH_BEND:
		m_pSynthesizer->setPitchbend (pMessage[1]);
		break;

	default:
		break;
	}
}