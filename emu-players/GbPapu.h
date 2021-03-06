/*
 * GbPapu.h
 *
 *  Created on: May 29, 2013
 *
 * Copyright 2013 Mic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GBPAPU_H_
#define GBPAPU_H_

#include "EmuCommon.h"
#include "Oscillator.h"
#include "GbZ80.h"

/* We're emulating the pAPU at 1/4th of its real speed, in order to save
 * processing power.
 */
#define GBPAPU_EMULATION_CLOCK (DMG_CLOCK/4)

class GbPapuChannel;
class GbPapuChip;


class GbPapuLengthCounter : public Oscillator
{
public:
	GbPapuLengthCounter() {}
	virtual ~GbPapuLengthCounter() {}

	virtual void Reset();
	virtual void Step();

	int GetMask() const;

	uint32_t mMax;
	bool mUse;
private:
	MAKE_NON_COPYABLE(GbPapuLengthCounter);
};


class GbPapuEnvelopeGenerator : public Oscillator
{
public:
	GbPapuEnvelopeGenerator() {}
	virtual ~GbPapuEnvelopeGenerator() {}

	virtual void Reset();
	virtual void Step();

	void SetChannel(GbPapuChannel *channel) { mChannel = channel; }

	GbPapuChannel *mChannel;
	uint32_t mMax;
	int16_t mDirection;
	bool mUse;
private:
	MAKE_NON_COPYABLE(GbPapuEnvelopeGenerator);
};


class GbPapuChannel : public Oscillator
{
public:
	virtual ~GbPapuChannel() {}

	virtual void Reset();
	virtual void Step();
	virtual void Write(uint32_t addr, uint8_t val);

	void SetChip(GbPapuChip *chip) { mChip = chip; }
	void SetIndex(uint8_t index) { mIndex = index; }

	GbPapuChip *mChip;
	GbPapuLengthCounter mLC;
	GbPapuEnvelopeGenerator mEG;
	uint16_t mLfsr, mLfsrWidth;
	uint16_t mOutL, mOutR;
	uint16_t mVol, mCurVol;
	uint8_t mIndex;
	uint8_t mWaveStep;
	uint8_t mDuty;
	uint8_t mPhase;
};


class GbPapuChip
{
public:
	void Reset();
	void Step();
	void Write(uint32_t addr, uint8_t val);

	/**
	 * @return -1 if the given channel (0-3) is enabled AND should be sent to the left/right
	 *         output AND the channel's length counter is non-zero.
	 */
	int ChannelEnabledLeft(uint8_t index) const;
	int ChannelEnabledRight(uint8_t index) const;

	// Channel enumerators
	enum
	{
		PULSE1 = 0,
		PULSE2 = 1,
		WAVE = 2,
		NOISE = 3,
	};

	static const uint8_t SQUARE_WAVES[4][8];
	static const uint16_t VOL_TB[];

	GbPapuChannel mChannels[4];
	uint8_t mWaveformRAM[16];
	uint8_t mNR51, mNR52;
};

#endif /* GBPAPU_H_ */
