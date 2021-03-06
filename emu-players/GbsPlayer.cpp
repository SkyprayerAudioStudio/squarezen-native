/*
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

#define NLOG_LEVEL_VERBOSE 0

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include "NativeLogger.h"
#include "GbsPlayer.h"
#include "GbCommon.h"
#include "GbMemory.h"
#include "GbZ80.h"


GbsPlayer::GbsPlayer()
	: mBlipBufRight(NULL), mSynthRight(NULL)
{
	cart = NULL;
}

MusicPlayer *GbsPlayerFactory()
{
	return new GbsPlayer;
}


GbsPlayer::~GbsPlayer()
{
	delete [] cart;
	cart = NULL;

	delete mBlipBufRight;
	delete [] mSynthRight;

	mBlipBufRight = NULL;
	mSynthRight   = NULL;
}


MusicPlayer::Result GbsPlayer::Reset()
{
	delete [] cart;
	cart = NULL;

	delete mBlipBuf;
	delete [] mSynth;
	mBlipBuf = NULL;
	mSynth   = NULL;

	delete mBlipBufRight;
	delete [] mSynthRight;
	mBlipBufRight = NULL;
	mSynthRight   = NULL;

	mState = MusicPlayer::STATE_CREATED;

	return MusicPlayer::OK;
}


void GbsPlayer::ExecuteGbZ80(uint16_t address)
{
	cart[0xF0] = 0xCD;	// CALL nn nn
	cart[0xF1] = address & 0xFF;
	cart[0xF2] = address >> 8;
	cart[0xF3] = 0x76;	// HALT
	cpu.regs.PC = 0xF0;
	cpu.cycles  = 0;
	cpu.stopped = 0;
	cpu_execute(mFrameCycles*2);
}


MusicPlayer::Result GbsPlayer::Prepare(std::string fileName)
{
	uint32_t  i;
    size_t fileSize;

	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    musicFile.read((char*)&mFileHeader, 0x70);
    NLOGD("GbsPlayer", "Reading GBS header");
    NLOGD("GbsPlayer", "ID: %c%c%c", mFileHeader.ID[0], mFileHeader.ID[1], mFileHeader.ID[2]);
    NLOGD("GbsPlayer", "Load: %#x\nInit: %#x\nPlay: %#x",
    		mFileHeader.loadAddress, mFileHeader.initAddress, mFileHeader.playAddress);
    NLOGD("GbsPlayer", "Timer control %#x, timer modulo %#x", mFileHeader.timerCtrl, mFileHeader.timerMod);

    numBanks = ((fileSize + mFileHeader.loadAddress - 0x70) + 0x3fff) >> 14;

    NLOGD("GbsPlayer", "Trying to allocate %d bytes (filesize = %d)", (uint32_t)numBanks << 14, fileSize);

    cart = new unsigned char[(uint32_t)numBanks << 14];

    memset(cart, 0, (uint32_t)numBanks << 14);
	musicFile.read((char*)cart + mFileHeader.loadAddress, fileSize-0x70);
	if (!musicFile) {
		NLOGE("GbsPlayer", "Read failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

	NLOGD("GbsPlayer", "File read done");

	musicFile.close();

	{
		mBlipBuf = new Blip_Buffer();
		mSynth = new Blip_Synth<blip_low_quality,4096>[4];

		if (mBlipBuf->set_sample_rate(44100)) {
			NLOGE("GbsPlayer", "Failed to set blipbuffer sample rate");
			return MusicPlayer::ERROR_UNKNOWN;
		}
		mBlipBuf->clock_rate(GBPAPU_EMULATION_CLOCK);

		for (i = 0; i < 4; i++) {
			//mSynth[i].volume(0.22);
			mSynth[i].output(mBlipBuf);
		}
	}
	{
		mBlipBufRight = new Blip_Buffer();
		mSynthRight = new Blip_Synth<blip_low_quality,4096>[4];

		if (mBlipBufRight->set_sample_rate(44100)) {
			//NativeLog("Failed to set blipbuffer sample rate");
			return MusicPlayer::ERROR_UNKNOWN;
		}
		mBlipBufRight->clock_rate(GBPAPU_EMULATION_CLOCK);

		for (i = 0; i < 4; i++) {
			//mSynthRight[i].volume(0.22);
			mSynthRight[i].output(mBlipBufRight);
		}
	}

	SetMasterVolume(0, 0);

	mFrameCycles = GBPAPU_EMULATION_CLOCK / 60;
	mCycleCount = 0;

	mem_set_papu(&mPapu);
	mem_set_gbsplayer(this);
	mem_reset();
	cpu_reset();
	mPapu.Reset();

	NLOGD("GbsPlayer", "Reset done");

	mFileHeader.title[31] = mFileHeader.author[31] = mFileHeader.copyright[31] = 0;
	mMetaData.SetTitle(mFileHeader.title);
	mMetaData.SetAuthor(mFileHeader.author);
	mMetaData.SetComment(mFileHeader.copyright);
	mMetaData.SetNumSubSongs(mFileHeader.numSongs);
	mMetaData.SetDefaultSong(mFileHeader.firstSong);

	SetSubSong(mFileHeader.firstSong - 1);

	NLOGD("GbsPlayer", "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;

	//SetSubSong(mFileHeader.firstSong);

	return MusicPlayer::OK;
}


void GbsPlayer::SetSubSong(uint32_t subSong)
{
	NLOGD("GbsPlayer", "Setting subsong %d", subSong);
	cpu.regs.SP = mFileHeader.SP;
	cpu.regs.A = subSong; // song number
	//cpu.regs.B = cpu.regs.C = cpu.regs.H = cpu.regs.L = 0;
	//mem_reset();
	cpu.halted = 0;
	ExecuteGbZ80(mFileHeader.initAddress);
}


void GbsPlayer::SetMasterVolume(int left, int right)
{
	for (int i = 0; i < 4; i++) {
		mSynth[i].volume(0.0314f * (float)left);
		mSynthRight[i].volume(0.0314f * (float)right);
	}
}


/*void GbsPlayer::PresentBuffer(int16_t *out, Blip_Buffer *inL, Blip_Buffer *inR)
{
	int count = inL->samples_avail();

	inL->read_samples(out, count, 1);
	inR->read_samples(out+1, count, 1);
}*/


MusicPlayer::Result GbsPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	int k;
	int blipLen = mBlipBuf->count_clocks(numSamples);
	int16_t out, outL, outR;

    if (MusicPlayer::STATE_PREPARED != GetState()) {
    	return MusicPlayer::ERROR_BAD_STATE;
    }

	for (k = 0; k < blipLen; k++) {
		if (mCycleCount == 0) {
			//NLOGV("GbsPlayer", "Running GBZ80 %d cycles", mFrameCycles);
			cpu.halted = 0;
			ExecuteGbZ80(mFileHeader.playAddress);
		}

		mPapu.Step();

		for (int i = 0; i < 4; i++) {
			out = (-mPapu.mChannels[i].mPhase) &
				  GbPapuChip::VOL_TB[mPapu.mChannels[i].mCurVol & 0x0F];
			outL = out & mPapu.ChannelEnabledLeft(i);
			outR = out & mPapu.ChannelEnabledRight(i);

			if (outL != mPapu.mChannels[i].mOutL) {
				mSynth[i].update(k, outL);
				mPapu.mChannels[i].mOutL = outL;
			}
			if (outR != mPapu.mChannels[i].mOutR) {
				mSynthRight[i].update(k, outR);
				mPapu.mChannels[i].mOutR = outR;
			}
		}

		mCycleCount++;
		if (mCycleCount == mFrameCycles) {
			mCycleCount = 0;
		}
	}

	mBlipBuf->end_frame(blipLen);
	mBlipBufRight->end_frame(blipLen);
	PresentBuffer(buffer, mBlipBuf, mBlipBufRight);

	return MusicPlayer::OK;
}


void GbsPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}

