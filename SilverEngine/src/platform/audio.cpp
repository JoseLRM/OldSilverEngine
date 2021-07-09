#include "platform/audio.h"

#include "utils/allocators.h"
#include "platform/os.h"

#include <windows.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#pragma comment(lib,"xaudio2.lib")

namespace sv {

	struct SoundInternal {
		RawList data;
		WAVEFORMATEX wave_format;
	};

	static bool create_sound_asset(void* asset, const char* name);
    static bool load_sound_asset(void* asset, const char* name, const char* filepath);
    static bool destroy_sound_asset(void* asset, const char* name);
    static bool reload_sound_asset(void* asset, const char* name, const char* filepath);

	struct AudioSource {
		IXAudio2SourceVoice* source;
		XAUDIO2_BUFFER buffer;

		Sound sound;
	};

	struct AudioState {

		IXAudio2* engine;
		IXAudio2MasteringVoice* mastering_voice;
		IXAudio2SubmixVoice* submix;

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

		// Register sound asset

		AssetTypeDesc desc;
		const char* extensions[5u];
		desc.extensions = extensions;
		
		extensions[0] = "wav";

		desc.name = "Sound";
		desc.asset_size = sizeof(SoundInternal);
		desc.extension_count = 1u;
		desc.create_fn = create_sound_asset;
		desc.load_file_fn = load_sound_asset;
		desc.free_fn = destroy_sound_asset;
		desc.reload_file_fn = reload_sound_asset;
		desc.unused_time = 3.f;

		SV_CHECK(register_asset_type(&desc));

		return true;
	}
	
	void _audio_close()
	{
		if (audio) {
			audio->source_allocator.clear();
			audio->submix->DestroyVoice();

			if (audio->mastering_voice)
				audio->mastering_voice->DestroyVoice();

			if (audio->engine)
				audio->engine->StopEngine();

			CoUninitialize();
			SV_FREE_STRUCT(audio);
			audio = NULL;
		}
	}

	void audio_source_create(AudioSource** source_)
	{
		AudioSource& source = audio->source_allocator.create();
		source.source = NULL;
		*source_ = &source;
	}
	
	void audio_source_destroy(AudioSource* source)
	{
		if (source->source)
			source->source->DestroyVoice();
		
		audio->source_allocator.destroy(*source);
	}

	SV_AUX SoundInternal* get_sound(Sound& asset)
	{
		void* ptr = get_asset_content(asset);
		return reinterpret_cast<SoundInternal*>(ptr);
	}

	void audio_source_set(AudioSource* source, Sound& sound_asset)
	{
		if (source->source) {
			source->source->DestroyVoice();
		}

		SoundInternal* sound = get_sound(sound_asset);

		if (sound == NULL) return;
		
		XAUDIO2_SEND_DESCRIPTOR send_descriptor = { 0, audio->submix };
		XAUDIO2_VOICE_SENDS send_list = { 1, &send_descriptor };

		HRESULT res = audio->engine->CreateSourceVoice(&source->source, &sound->wave_format, 0U, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &send_list, nullptr);
		if (FAILED(res)) {
			return;
		}

		source->buffer.pAudioData = sound->data.data();
		source->buffer.AudioBytes = (u32)sound->data.size();
		source->buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		source->buffer.Flags = XAUDIO2_END_OF_STREAM;

		res = source->source->SubmitSourceBuffer(&source->buffer, nullptr);
		if (FAILED(res)) {
			return;
		}

		source->sound = sound_asset;
	}

	static bool create_sound_asset(void* asset, const char* name)
    {
		new(asset) SoundInternal();
		return true;
    }

    static bool load_sound_asset(void* asset, const char* name, const char* filepath)
    {
		SoundInternal& sound = *new(asset) SoundInternal();

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

		return true;
    }

    static bool destroy_sound_asset(void* asset, const char* name)
    {
		SoundInternal& sound = *reinterpret_cast<SoundInternal*>(asset);
		sound.~SoundInternal();
		return true;
    }

    static bool reload_sound_asset(void* asset, const char* name, const char* filepath)
    {
		SV_CHECK(destroy_sound_asset(asset, name));
		return load_sound_asset(asset, name, filepath);
    }

	void audio_play(AudioSource* source)
	{
		if (source->source) {
			source->source->Start();
		}
	}

	void audio_pause(AudioSource* source)
	{
		if (source->source) {
			source->source->Stop();
		}
	}
	void audio_stop(AudioSource* source)
	{
		if (source->source) {
			//source->source->SubmitSource();
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
