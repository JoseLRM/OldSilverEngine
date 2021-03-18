#include "SilverEngine/core.h"

#include "SilverEngine/audio.h"
#include "SilverEngine/utils/allocators/InstanceAllocator.h"

#include <windows.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#pragma comment(lib,"xaudio2.lib")

namespace sv {

	struct Sound_internal {
		std::vector<u8> data;
		WAVEFORMATEX wave_format;
	};

	struct {

		IXAudio2* engine;
		IXAudio2MasteringVoice* mastering_voice;

		InstanceAllocator<Sound_internal, 100u> sound_allocator;

	} audio;

	struct AudioSource_internal {

		IXAudio2SourceVoice* source;
		XAUDIO2_BUFFER buffer;

	};

	struct AudioDevice_internal {

		IXAudio2SubmixVoice* submix;

		InstanceAllocator<AudioSource_internal, 30u> source_allocator;

	};

#define PARSE_DEVICE() sv::AudioDevice_internal& device = *reinterpret_cast<sv::AudioDevice_internal*>(device_)
#define PARSE_SOURCE() sv::AudioSource_internal& source = *reinterpret_cast<sv::AudioSource_internal*>(source_)
#define PARSE_SOUND() sv::Sound_internal& sound = *reinterpret_cast<sv::Sound_internal*>(sound_)

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

	Result initialize_audio()
	{
		HRESULT res;
		res = CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(res)) {
			return Result_UnknownError;
		}

		res = XAudio2Create(&audio.engine, 0);
		if (FAILED(res)) {
			SV_LOG_ERROR("Can't initialize xaudio");
			return Result_UnknownError;
		}

		audio.engine->CreateMasteringVoice(&audio.mastering_voice);
		if (audio.mastering_voice == nullptr) {
			SV_LOG_ERROR("Can't create mastering voice");
			return Result_UnknownError;
		}

		return Result_Success;
	}

	Result close_audio()
	{
		u32 unfreed_sounds = audio.sound_allocator.unfreed_count();
		if (unfreed_sounds) {
			SV_LOG_WARNING("There are %u unfreed sounds", unfreed_sounds);
		}
		audio.sound_allocator.clear();

		if (audio.mastering_voice)
			audio.mastering_voice->DestroyVoice();

		if (audio.engine)
			audio.engine->StopEngine();

		CoUninitialize();

		return Result_Success;
	}

	Result create_audio_device(AudioDevice** pdevice)
	{
		AudioDevice_internal& device = *new AudioDevice_internal();
		// TODO: Handle memory if fails the creation

		HRESULT res = audio.engine->CreateSubmixVoice(&device.submix, 1u, 44100u);
		if (FAILED(res)) 
			return Result_UnknownError;

		*pdevice = (AudioDevice*) &device;
		return Result_Success;
	}

	Result destroy_audio_device(AudioDevice* device_)
	{
		PARSE_DEVICE();

		u32 unfreed_sources = device.source_allocator.unfreed_count();
		if (unfreed_sources) {
			SV_LOG_WARNING("There are unfreed sources");

			for (auto& pool : device.source_allocator) {
				for (AudioSource_internal& source : pool) {
					source.source->DestroyVoice();
				}
			}

			device.source_allocator.clear();
		}

		device.submix->DestroyVoice();

		delete& device;
		return Result_Success;
	}

	Result create_audio_source(AudioDevice* device_, Sound* sound_, AudioSource** psource)
	{
		PARSE_DEVICE();
		PARSE_SOUND();
		AudioSource_internal& source = device.source_allocator.create();
		// TODO: Handle memory if fails the creation

		XAUDIO2_SEND_DESCRIPTOR send_descriptor = { 0, device.submix };
		XAUDIO2_VOICE_SENDS send_list = { 1, &send_descriptor };

		HRESULT res = audio.engine->CreateSourceVoice(&source.source, &sound.wave_format, 0U, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &send_list, nullptr);
		if (FAILED(res)) {
			return Result_UnknownError;
		}

		source.buffer.pAudioData = sound.data.data();
		source.buffer.AudioBytes = sound.data.size();
		source.buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		source.buffer.Flags = XAUDIO2_END_OF_STREAM;

		res = source.source->SubmitSourceBuffer(&source.buffer, nullptr);
		if (FAILED(res)) {
			return Result_UnknownError;
		}

		*psource = (AudioSource*)&source;
		return Result_Success;
	}

	Result destroy_audio_source(AudioDevice* device_, AudioSource* source_)
	{
		PARSE_DEVICE();
		PARSE_SOURCE();

		source.source->DestroyVoice();
		device.source_allocator.destroy(source);

		return Result_Success;
	}

	Result create_sound(const char* filepath, Sound** psound)
	{
		Sound_internal& sound = audio.sound_allocator.create();

		std::vector<u8> data;
		svCheck(file_read_binary(filepath, data));

		DWORD chunk_size;
		DWORD chunk_position;
		bool found = find_chunk(data.data(), fourccRIFF, chunk_size, chunk_position);
		if (found) {

			u8* chunk_data = data.data() + chunk_position;
			
			DWORD file_type;
			memcpy(&file_type, chunk_data, sizeof(DWORD));
			if (file_type != fourccWAVE) {
				return Result_InvalidFormat;
			}

			// Load wave format

			found = find_chunk(data.data(), fourccFMT, chunk_size, chunk_position);
			if (found) {

				chunk_data = data.data() + chunk_position;
				SV_ZERO_MEMORY(&sound.wave_format, sizeof(sound.wave_format));
				memcpy(&sound.wave_format, chunk_data, chunk_size);
			}
			else {
				return Result_InvalidFormat;
			}

			// Load wave data
			found = find_chunk(data.data(), fourccDATA, chunk_size, chunk_position);
			if (found) {

				chunk_data = data.data() + chunk_position;
			}
			else {
				return Result_InvalidFormat;
			}

			sound.data.resize(chunk_size);
			memcpy(sound.data.data(), chunk_data, chunk_size);
		}
		else {
			return Result_TODO;
		}

		*psound = (Sound*)& sound;
		return Result_Success;
	}

	Result destroy_sound(Sound* sound_)
	{
		PARSE_SOUND();

		audio.sound_allocator.destroy(sound);
		return Result_Success;
	}

	void play_sound(AudioSource* source_)
	{
		PARSE_SOURCE();
		if (source.source) {
			source.source->Start();
		}
	}

	void set_audio_volume(AudioDevice* device_, f32 volume)
	{
		PARSE_DEVICE();
		device.submix->SetVolume(volume);
	}

	void set_audio_volume(AudioSource* source_, f32 volume)
	{
		PARSE_SOURCE();
		source.source->SetVolume(volume);
	}

	f32 get_audio_volume(AudioDevice* device_)
	{
		PARSE_DEVICE();
		f32 volume;
		device.submix->GetVolume(&volume);
		return volume;
	}

	f32 get_audio_volume(AudioSource* source_)
	{
		PARSE_SOURCE();
		f32 volume;
		source.source->GetVolume(&volume);
		return volume;
	}

}