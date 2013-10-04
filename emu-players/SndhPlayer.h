/*
 * SgcPlayer.h
 *
 *  Created on: Oct 4, 2013
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

#ifndef SNDHPLAYER_H_
#define SNDHPLAYER_H_

#include <string>
#include <stdint.h>
#include "MusicPlayer.h"
#include "M68000.h"
#include "YM2149.h"
#include "SndhMapper.h"


class SndhPlayer : public MusicPlayer
{
public:
	SndhPlayer();
	virtual ~SndhPlayer();

	virtual MusicPlayer::Result Prepare(std::string fileName);
	virtual MusicPlayer::Result Run(uint32_t numSamples, int16_t *buffer);
	virtual MusicPlayer::Result Reset();

	typedef struct __attribute__ ((__packed__))
	{
		uint32_t initDriverInstruction;		// e.g. BRA.W init_music_driver  (big-endian)
		uint32_t exitDriverInstruction;
		uint32_t vblPlayInstruction;
		char signature[4];					// "SNDH"
	} SndhFileHeader;

private:
	static int ReadString(std::ifstream& sndhFile, char *buffer, size_t maxChars);
	MusicPlayer::Result ParseTags(std::ifstream& sndhFile);

	SndhFileHeader mFileHeader;
	M68000 *m68k;
	YmChip *mYm;
	SndhMapper *mMemory;
};

MusicPlayer *SndhPlayerFactory();

#endif	/* SNDHPLAYER_H_ */