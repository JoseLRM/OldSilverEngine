#include "SilverEngine.h"

using namespace sv;

Result app_validate_scene(const char* name)
{
	const char* valid_names[] = {
			"Test"
	};

	foreach(i, sizeof(valid_names) / sizeof(const char*)) {

		if (strcmp(name, valid_names[i]) == 0) {

			return Result_Success;
		}
	}

	return Result_NotFound;
}

Result app_init_scene(Scene* scene, Archive* archive)
{
	if (archive == nullptr) {
	
		Entity cam = create_entity(scene, SV_ENTITY_NULL, "Camera");
		scene->main_camera = cam;
		CameraComponent* camera = add_component<CameraComponent>(scene, cam);
		camera->far = 10000.f;
		camera->near = 0.2f;
		camera->width = 0.5f;
		camera->height = 0.5f;
		camera->projection_type = ProjectionType_Perspective;
	}

	svCheck(load_skymap_image(SV_SYS("system/skymap.jpg"), &scene->skybox));

	return Result_Success;
}

Result app_close_scene(Scene* scene)
{

	graphics_destroy(scene->skybox);

	return Result_Success;
}

AudioSource* source = nullptr;
Sound* s;

Result init()
{
	set_active_scene("Test");

	create_sound("On the nature of daylight.WAV", &s);

	return Result_Success;
}

void update()
{
	if (source == nullptr) {

		create_audio_source(engine.scene->audio_device, s, &source);
		play_sound(source);
	}

	if (input.keys[Key_Up]) {

		f32 volume = get_audio_volume(source);
		set_audio_volume(source, std::min(volume + 0.1f * engine.deltatime, 1.f));

	}
	if (input.keys[Key_Down]) {

		f32 volume = get_audio_volume(source);
		set_audio_volume(source, std::max(volume - 0.1f * engine.deltatime, 0.f));

	}
}

void update_scene(Scene* scene)
{
}

Result close()
{

	return Result_Success;
}

std::string get_scene_filepath(const char* name)
{
	std::stringstream ss;
	ss << "scenes/";
	ss << name << ".scene";
	return ss.str();
}

int main()
{
	InitializationDesc desc;
	desc.minThreadsCount = 2u;
	desc.callbacks.initialize = init;
	desc.callbacks.update = update;
	desc.callbacks.close = close;

	desc.callbacks.validate_scene = app_validate_scene;
	desc.callbacks.initialize_scene = app_init_scene;
	desc.callbacks.update_scene = update_scene;
	desc.callbacks.close_scene = app_close_scene;
	desc.callbacks.serialize_scene = nullptr;
	desc.callbacks.get_scene_filepath = get_scene_filepath;

	desc.windowDesc.bounds = { 0u, 0u, 1080u, 720u };
	desc.windowDesc.flags = WindowFlag_Default;
	desc.windowDesc.iconFilePath = nullptr;
	//desc.windowDesc.state = WindowState_Windowed;
	desc.windowDesc.state = WindowState_Fullscreen;
	desc.windowDesc.title = L"Test";

	if (result_fail(engine_initialize(&desc))) {
		printf("Can't init");
	}
	else {

		while (result_okay(engine_loop()));

		engine_close();
	}

	system_pause();

}
