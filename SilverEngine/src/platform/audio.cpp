#include "platform/audio.h"

#include "utils/allocators.h"
#include "platform/os.h"

#include <windows.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#pragma comment(lib,"xaudio2.lib")

namespace sv {

	struct Sound {
		RawList data;
		WAVEFORMATEX wave_format;
	};

	struct AudioSource {
		IXAudio2SourceVoice* source;
		XAUDIO2_BUFFER buffer;
	};

	struct AudioState {

		IXAudio2* engine;
		IXAudio2MasteringVoice* mastering_voice;
		IXAudio2SubmixVoice* submix;

		InstanceAllocator<Sound, 100u> sound_allocator;
		InstanceAllocator<AudioSource, 30u> source_allocator;

	};

	static AudioState* audio;

#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif
	
	bool find_chunk(u8* data, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
	{
		size_t pos = 0u;

		DWORD chunk_type;
		DWORD chunk_size;
		DWORD riff_data_size = 0;
		DWORD file_type;
		DWORD bytesRead = 0;
		DWORD off = 0;

		while (true)
		{
			memcpy(&chunk_type, data + pos, sizeof(DWORD));
			pos += sizeof(DWORD);

			memcpy(&chunk_size, data + pos, sizeof(DWORD));
			pos += sizeof(DWORD);

			switch (chunk_type)
			{
			case fourccRIFF:
				riff_data_size = chunk_size;
				chunk_size = 4;

				memcpy(&file_type, data + pos, sizeof(DWORD));
				pos += sizeof(DWORD);
				break;

			default:
				pos += chunk_size;
			}

			off += sizeof(DWORD) * 2;

			if (chunk_type == fourcc)
			{
				dwChunkSize = chunk_size;
				dwChunkDataPosition = off;
				return true;
			}

			off += chunk_size;

			if (bytesRead >= riff_data_size) return false;

		}

		return true;
	}

	bool _audio_initialize()
	{
		audio = SV_ALLOCATE_STRUCT(AudioState, "Audio");
		
		HRESULT res;
		res = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(res)) {
			return false;
		}

		res = XAudio2Create(&audio->engine, 0);
		if (FAILED(res)) {
			SV_LOG_ERROR("Can't initialize xaudio");
			return false;
		}

		audio->engine->CreateMasteringVoice(&audio->mastering_voice);
		if (audio->mastering_voice == nullptr) {
			SV_LOG_ERROR("Can't create mastering voice");
			return false;
		}

		res = audio->engine->CreateSubmixVoice(&audio->submix, 1u, 44100u);
		if (FAILED(res)) 
			return false;

		return true;
	}
	
	void _audio_close()
	{
		audio->source_allocator.clear();
		audio->submix->DestroyVoice();
		
		audio->sound_allocator.clear();

		if (audio->mastering_voice)
			audio->mastering_voice->DestroyVoice();

		if (audio->engine)
			audio->engine->StopEngine();

		CoUninitialize();
	}

	bool audio_source_create(AudioSource** source_, Sound* sound)
	{
		AudioSource& source = audio->source_allocator.create();
		// TODO: Handle memory if fails the creation

		XAUDIO2_SEND_DESCRIPTOR send_descriptor = { 0, audio->submix };
		XAUDIO2_VOICE_SENDS send_list = { 1, &send_descriptor };

		HRESULT res = audio->engine->CreateSourceVoice(&source.source, &sound->wave_format, 0U, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &send_list, nullptr);
		if (FAILED(res)) {
			return false;
		}

		source.buffer.pAudioData = sound->data.data();
		source.buffer.AudioBytes = (u32)sound->data.size();
		source.buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		source.buffer.Flags = XAUDIO2_END_OF_STREAM;

		res = source.source->SubmitSourceBuffer(&source.buffer, nullptr);
		if (FAILED(res)) {
			return false;
		}

		*source_ = &source;
		return true;
	}
	
	void audio_source_destroy(AudioSource* source)
	{
		source->source->DestroyVoice();
		audio->source_allocator.destroy(*source);
	}

	bool audio_sound_load(Sound** sound_, const char* filepath)
	{
		Sound& sound = audio->sound_allocator.create();

		RawList data;
		SV_CHECK(file_read_binary(filepath, data));

		DWORD chunk_size;
		DWORD chunk_position;
		bool found = find_chunk(data.data(), fourccRIFF, chunk_size, chunk_position);
		if (found) {

			u8* chunk_data = data.data() + chunk_position;
			
			DWORD file_type;
			memcpy(&file_type, chunk_data, sizeof(DWORD));
			if (file_type != fourccWAVE) {
				return false;
			}

			// Load wave format

			found = find_chunk(data.data(), fourccFMT, chunk_size, chunk_position);
			if (found) {

				chunk_data = data.data() + chunk_position;
				SV_ZERO_MEMORY(&sound.wave_format, sizeof(sound.wave_format));
				memcpy(&sound.wave_format, chunk_data, chunk_size);
			}
			else {
				return false;
			}

			// Load wave data
			found = find_chunk(data.data(), fourccDATA, chunk_size, chunk_position);
			if (found) {

				chunk_data = data.data() + chunk_position;
			}
			else {
				return false;
			}

			sound.data.resize(chunk_size);
			memcpy(sound.data.data(), chunk_data, chunk_size);
		}
		else {
			// TODO
			return false;
		}

		*sound_ = &sound;
		return true;
	}
	
	void audio_sound_destroy(Sound* sound)
	{
		audio->sound_allocator.destroy(*sound);
	}

	void audio_play(AudioSource* source)
	{
		if (source->source) {
			source->source->Start();
		}
	}

	void audio_global_volume_set(f32 volume)
	{
		audio->submix->SetVolume(volume);
	}
	
	void audio_volume_set(AudioSource* source, f32 volume)
	{
		source->source->SetVolume(volume);
	}

	f32 audio_global_volume_get()
	{
		f32 volume;
		audio->submix->GetVolume(&volume);
		return volume;
	}
	
	f32 audio_volume_get(AudioSource* source)
	{
		f32 volume;
		source->source->GetVolume(&volume);
		return volume;
	}
	
}
