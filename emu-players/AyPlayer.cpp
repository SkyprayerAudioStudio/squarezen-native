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
#define NLOG_TAG "AyPlayer"

#include <iostream>
#include <fstream>
#include "NativeLogger.h"
#include "AyPlayer.h"


AyPlayer::AyPlayer()
{

}


MusicPlayer *AyPlayerFactory()
{
	return new AyPlayer;
}


AyPlayer::~AyPlayer()
{

}

MusicPlayer::Result AyPlayer::Reset()
{
	// ToDo: implement
	NLOGV(NLOG_TAG, "Reset");
	mState = MusicPlayer::STATE_CREATED;
	return MusicPlayer::OK;
}

MusicPlayer::Result AyPlayer::Prepare(std::string fileName)
{
	size_t fileSize;

	NLOGV(NLOG_TAG, "Prepare(%s)", fileName.c_str());
	(void)MusicPlayer::Prepare(fileName);

	MusicPlayer::Result result;
    std::ifstream musicFile;
    if (MusicPlayer::OK != (result = OpenFile(musicFile, fileName, fileSize))) {
    	return result;
    }

    NLOGV(NLOG_TAG, "Reading header");
    musicFile.read((char*)&mFileHeader, sizeof(mFileHeader));
	if (!musicFile) {
		NLOGE(NLOG_TAG, "Reading AY file header failed");
        musicFile.close();
		return MusicPlayer::ERROR_FILE_IO;
	}

    if (strncmp(mFileHeader.ID, "ZXAY", 4) || strncmp(mFileHeader.typeID, "EMUL", 4)) {
    	NLOGE(NLOG_TAG, "Unknown AY header ID");
    	musicFile.close();
    	return MusicPlayer::ERROR_UNRECOGNIZED_FORMAT;
    }

    // ToDo: byteswap offsets


    for (int i = 0; i < mFileHeader.numSongs; i++) {
    	// ToDo: read song structs
    }

    // ToDo: finish

	NLOGV(NLOG_TAG, "File read done");
	musicFile.close();

	NLOGD(NLOG_TAG, "Prepare finished");

	mState = MusicPlayer::STATE_PREPARED;
	return MusicPlayer::OK;
}


MusicPlayer::Result AyPlayer::Run(uint32_t numSamples, int16_t *buffer)
{
	// ToDo: implement
	return MusicPlayer::OK;
}

void AyPlayer::GetChannelOutputs(int16_t *outputs) const
{
	// ToDo: implement
}


