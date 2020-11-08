#include "core_editor.h"

#include "scene_editor.h"
#include "panel_manager.h"
#include "panels/SceneEditorPanel.h"
#include "platform/input.h"
#include "simulation/scene.h"
#include "simulation.h"
#include "rendering/debug_renderer.h"

namespace sv {

	static SceneEditorMode g_EditorMode = SceneEditorMode(ui32_max);

	static DebugCamera g_Camera;

	static RendererDebugBatch* g_DebugBatch;

	// Camera movement

	static bool g_CameraInMovement = false;
	static struct {
		
		vec2f initialMovement;

	} g_CameraMovementData;

	// Action

	enum ActionMode {

		ActionMode_None,
		ActionMode_Transform,
		ActionMode_Selection,

	};

	static vec2f g_MousePos;
	static bool g_MouseInCamera = false;

	static ActionMode g_ActionMode = ActionMode_None;
	static bool g_ActionStart = false;
	static struct {

		vec2f mousePos;

	} g_ActionStartData;

	static Entity g_SelectedEntity = SV_ENTITY_NULL;

	// Transform Action
	
	enum TransformMode : ui32 {

		TransformMode_Translation,
		TransformMode_Scale,
		TransformMode_Rotation,

	};
	
	static TransformMode g_TransformMode = TransformMode_Translation;

	struct {

		vec2f beginMousePos;
		vec2f beginPos;
		vec2f beginScale;
		float beginRotation;
		bool selectedXAxis;
		bool selectedYAxis;

	} g_TransformModeData;

	// CAMERA CONTROLLERS

	void scene_editor_camera_controller_2D(const vec2f mousePos, float dt)
	{
		auto& cam = g_Camera.camera;
		float length = cam.getProjectionLength();

		// Movement
		if (input_mouse_pressed(SV_MOUSE_CENTER)) {
			g_CameraInMovement = true;

			g_CameraMovementData.initialMovement = mousePos;
		}
		else if (input_mouse_released(SV_MOUSE_CENTER)) {
			g_CameraInMovement = false;
		}

		if (g_CameraInMovement) {

			g_Camera.position += (g_CameraMovementData.initialMovement - mousePos).get_vec3();

		}

		// Zoom
		length += -input_mouse_wheel_get() * length * 0.1f;
		if (length < 0.001f) length = 0.001f;

		cam.setProjectionLength(length);

	}

	void scene_editor_camera_controller_3D(float dt)
	{
		vec2f dragged = input_mouse_dragged_get();
		
		if (input_mouse(SV_MOUSE_RIGHT)) {
			
			dragged *= 700.f;

			g_Camera.rotation.x += ToRadians(dragged.y);
			g_Camera.rotation.y += ToRadians(dragged.x);

		}
		else if (input_mouse(SV_MOUSE_CENTER)) {

			if (dragged.length() != 0.f) {
				auto& cam = g_Camera.camera;
				dragged *= { cam.getWidth(), cam.getHeight() };
				XMVECTOR lookAt = XMVectorSet(dragged.x, dragged.y, 0.f, 0.f);
				lookAt = XMVector3Transform(lookAt, XMMatrixRotationRollPitchYawFromVector(g_Camera.rotation.get_dx()));
				g_Camera.position -= vec3f(lookAt) * 4000.f;
			}
		}

		float scroll = 0.f;

		if (input_key('W')) {
			scroll += dt;
		}
		if (input_key('S')) {
			scroll -= dt;
		}

		if (scroll != 0.f) {
			XMVECTOR lookAt = XMVectorSet(0.f, 0.f, 1.f, 0.f);
			lookAt = XMVector3Transform(lookAt, XMMatrixRotationRollPitchYawFromVector(g_Camera.rotation.get_dx()));
			lookAt = XMVector3Normalize(lookAt);

			vec3f svlookAt = vec3f(lookAt);
			svlookAt *= scroll * 100.f;
			g_Camera.position += svlookAt;
		}

	}

	// MAIN FUNCTIONS
	Result scene_editor_initialize()
	{
		// Create camera
		{
			Scene& scene = simulation_scene_get();
			Entity camera = scene.getMainCamera();

			SV_ASSERT(ecs_entity_exist(scene, camera));
			Camera& mainCamera = ecs_component_get<CameraComponent>(scene, camera)->camera;
			vec2u res = { mainCamera.getResolutionWidth(), mainCamera.getResolutionHeight() };
			g_Camera.camera.setResolution(res.x, res.y);

			g_Camera.position = { 0.f, 0.f, -10.f };

			scene_editor_mode_set(SceneEditorMode_2D);

			svCheck(debug_renderer_batch_create(&g_DebugBatch));
		}
		return Result_Success;
	}

	Result scene_editor_close()
	{
		g_Camera.camera.clear();
		svCheck(debug_renderer_batch_destroy(g_DebugBatch));
		return Result_Success;
	}

	bool scene_editor_is_entity_in_point(ECS* ecs, Entity entity, const vec2f& point)
	{
		Transform trans = ecs_entity_transform_get(ecs, entity);

		vec2f pos = trans.getWorldPosition().get_vec2();
		float rot = trans.getWorldEulerRotation().z;
		vec2f auxMousePos = point - pos;
		rot -= auxMousePos.angle();
		auxMousePos.setAngle(rot);
		auxMousePos += pos;

		BoundingBox2D aabb;
		aabb.init_center(pos, trans.getWorldScale().get_vec2());

		return aabb.intersects_point(auxMousePos);
	}

	std::vector<Entity> scene_editor_entities_in_point(const vec2f& point)
	{
		SceneEditorPanel* vp = (SceneEditorPanel*)panel_manager_get("Scene Editor");
		Scene& scene = simulation_scene_get();
		std::vector<Entity> selectedEntities;

		ui32 entityCount = ecs_entity_count(scene);

		for (ui32 i = 0; i < entityCount; ++i) {

			Entity entity = ecs_entity_get(scene, i);

			if (scene_editor_is_entity_in_point(scene, entity, point)) {
				selectedEntities.push_back(entity);
			}
		}

		return selectedEntities;
	}

	void scene_editor_update_mode2D(float dt)
	{
		SceneEditorPanel* vp = (SceneEditorPanel*)panel_manager_get("Scene Editor");
		Scene& scene = simulation_scene_get();

		// Transform mode
		if (input_key_pressed('T')) g_TransformMode = TransformMode_Translation;
		if (input_key_pressed('R')) g_TransformMode = TransformMode_Scale;
		if (input_key_pressed('E')) g_TransformMode = TransformMode_Rotation;

		// Compute Mouse Pos
		g_MousePos = input_mouse_position_get();
		vp->adjustCoord(g_MousePos);
		g_MouseInCamera = g_MousePos.x > -0.5f && g_MousePos.x < 0.5f && g_MousePos.y > -0.5f && g_MousePos.y < 0.5f;

		g_MousePos *= { g_Camera.camera.getWidth(), g_Camera.camera.getHeight() };
		g_MousePos += g_Camera.position.get_vec2();

		scene_editor_camera_controller_2D(g_MousePos, dt);

		// Actions

		if (g_MouseInCamera && input_mouse_pressed(SV_MOUSE_LEFT)) {
			g_ActionStart = true;
			g_ActionMode = ActionMode_None;

			g_ActionStartData.mousePos = g_MousePos;
		}

		if (g_ActionStart && g_MouseInCamera) {

			if (input_mouse_released(SV_MOUSE_LEFT)) {

				g_ActionStart = false;
				g_ActionMode = ActionMode_Selection;

			}

			else if (g_SelectedEntity != SV_ENTITY_NULL && input_mouse_dragged_get().length() != 0.f) {

				Transform trans = ecs_entity_transform_get(scene, g_SelectedEntity);

				vec3f pos = trans.getWorldPosition();
				vec3f scale = trans.getWorldScale();
				float rot = trans.getWorldEulerRotation().z;

				g_TransformModeData.beginPos = pos.get_vec2();
				g_TransformModeData.beginMousePos = g_ActionStartData.mousePos;
				g_TransformModeData.beginScale = scale.get_vec2();
				g_TransformModeData.beginRotation = rot;

				float maxScale = std::max(abs(scale.x), abs(scale.y));

				BoundingBox2D xAxis;
				xAxis.init_minmax(pos.get_vec2() + vec2f{ 0.f, maxScale * -0.05f }, pos.get_vec2() + vec2f{ maxScale * 0.6f, maxScale * 0.05f });
				
				BoundingBox2D yAxis;
				yAxis.init_minmax(pos.get_vec2() + vec2f{ maxScale * -0.05f, 0.f }, pos.get_vec2() + vec2f{ maxScale * 0.05f, maxScale * 0.6f });

				g_TransformModeData.selectedXAxis = xAxis.intersects_point(g_ActionStartData.mousePos);
				g_TransformModeData.selectedYAxis = yAxis.intersects_point(g_ActionStartData.mousePos);

				if (g_TransformModeData.selectedXAxis || g_TransformModeData.selectedYAxis || scene_editor_is_entity_in_point(scene, g_SelectedEntity, g_ActionStartData.mousePos)) {
					g_ActionStart = false;
					g_ActionMode = ActionMode_Transform;
				}
			}
		}

		switch (g_ActionMode)
		{
		case sv::ActionMode_Transform:
			// Transform
		{
			Transform trans = ecs_entity_transform_get(scene, g_SelectedEntity);
			auto& data = g_TransformModeData;

			switch (g_TransformMode)
			{
			case sv::TransformMode_Translation:
			{
				vec2f diference = data.beginPos - data.beginMousePos;
				vec2f pos = trans.getWorldPosition().get_vec2();

				pos = g_MousePos + diference;

				if (data.selectedXAxis) {
					trans.setPositionX(pos.x);
				}
				else if (data.selectedYAxis) {
					trans.setPositionY(pos.y);
				}
				else {
					trans.setPositionX(pos.x);
					trans.setPositionY(pos.y);
				}
			}
			break;
			case sv::TransformMode_Scale:
			{
				float dif = (g_MousePos - data.beginMousePos).length();
				if ((g_MousePos - data.beginPos).length() < (g_MousePos - data.beginMousePos).length()) dif = -dif;

				vec2f newScale = data.beginScale + dif;

				if (data.selectedXAxis) {
					trans.setScaleX(newScale.x);
				}
				else if (data.selectedYAxis) {
					trans.setScaleY(newScale.y);
				}
				else {
					trans.setScaleX(newScale.x);
					trans.setScaleY(newScale.y);
				}
			}
				break;
			case sv::TransformMode_Rotation:
				break;
			}

			if (input_mouse_released(SV_MOUSE_LEFT)) {
				g_ActionMode = ActionMode_None;
			}
		}
			break;
		case sv::ActionMode_Selection:
		{
			std::vector<Entity> selectedEntities = scene_editor_entities_in_point(g_MousePos);

			// Select the selected entity :)
			if (selectedEntities.size()) {
				Entity selected = selectedEntities.front();
				float depth = ecs_entity_transform_get(scene, selected).getWorldPosition().z;
				for (ui32 i = 1u; i < selectedEntities.size(); ++i) {
					Entity e = selectedEntities[i];

					if (e == g_SelectedEntity) continue;

					float eDepth = ecs_entity_transform_get(scene, e).getWorldPosition().z;
					if (eDepth > depth) {
						selected = e;
						depth = eDepth;
					}
				}

				g_SelectedEntity = selected;
			}
			else g_SelectedEntity = SV_ENTITY_NULL;

			g_ActionMode = ActionMode_None;
		}
			break;
		}

	}

	void scene_editor_update_mode3D(float dt)
	{
		scene_editor_camera_controller_3D(dt);
	}

	void scene_editor_update(float dt)
	{
		Scene& scene = simulation_scene_get();

		if (input_key(SV_KEY_CONTROL)) {

			if (g_SelectedEntity != SV_ENTITY_NULL && input_key_pressed('D')) {
				ecs_entity_duplicate(scene, g_SelectedEntity);
			}

		}

		if (g_SelectedEntity != SV_ENTITY_NULL && input_key_pressed(SV_KEY_DELETE)) {
			ecs_entity_destroy(scene, g_SelectedEntity);
			g_SelectedEntity = SV_ENTITY_NULL;
		}

		SceneEditorPanel* vp = (SceneEditorPanel*) panel_manager_get("Scene Editor");
		if (vp == nullptr) return;

		// Adjust camera
		vec2u size = vp->getScreenSize();
		if (size.x != 0u && size.y != 0u)
			g_Camera.camera.adjust(size.x, size.y);
		
		if (vp->isFocused()) {
			switch (g_EditorMode)
			{
			case sv::SceneEditorMode_2D:
				scene_editor_update_mode2D(dt);
				break;
			case sv::SceneEditorMode_3D:
				scene_editor_update_mode3D(dt);
				break;
			}
		}
	}

	void scene_editor_render()
	{
		SceneEditorPanel* vp = (SceneEditorPanel*)panel_manager_get("Scene Editor");
		if (vp == nullptr) return;

		if (!vp->isVisible()) {
			return;
		}

		Scene& scene = simulation_scene_get();

		scene.drawCamera(&g_Camera.camera, g_Camera.position, g_Camera.rotation);

		// DEBUG RENDERING

		XMMATRIX viewProjectionMatrix = math_matrix_view(g_Camera.position, g_Camera.rotation) * g_Camera.camera.getProjectionMatrix();
		CommandList cmd = graphics_commandlist_get();

		// Draw 2D Colliders
		{
			debug_renderer_batch_reset(g_DebugBatch);

			EntityView<RigidBody2DComponent> bodies(scene);

			debug_renderer_stroke_set(g_DebugBatch, 0.05f);

			for (RigidBody2DComponent& body : bodies) {

				{
					BoxCollider2DComponent* collider = ecs_component_get<BoxCollider2DComponent>(scene, body.entity);

					if (collider) {

						Transform trans = ecs_entity_transform_get(scene, body.entity);
						vec3f position = trans.getWorldPosition();
						vec3f scale = trans.getWorldScale();
						vec4f rotation = trans.getWorldRotation();

						XMVECTOR colliderPosition = XMVectorSet(position.x, position.y, position.z, 0.f);
						XMVECTOR colliderScale = XMVectorSet(scale.x, scale.y, 0.f, 0.f);
						colliderScale = XMVectorMultiply(XMVectorSet(collider->getSize().x * 2.f, collider->getSize().y * 2.f, 0.f, 0.f), colliderScale);

						XMVECTOR offset = XMVectorSet(collider->getOffset().x, collider->getOffset().y, 0.f, 0.f);
						offset = XMVector3Transform(offset, XMMatrixRotationQuaternion(rotation.get_dx()));

						colliderPosition += offset;

						debug_renderer_draw_quad(g_DebugBatch, vec3f(colliderPosition), vec2f(colliderScale), rotation, { 0u, 255u, 0u, 255u });
					}
				}
				{
					CircleCollider2DComponent* collider = ecs_component_get<CircleCollider2DComponent>(scene, body.entity);

					if (collider) {

						Transform trans = ecs_entity_transform_get(scene, body.entity);
						vec3f position = trans.getWorldPosition();
						vec3f scale = trans.getWorldScale();
						vec4f rotation = trans.getWorldRotation();

						XMVECTOR colliderPosition = XMVectorSet(position.x, position.y, position.z, 0.f);
						XMVECTOR colliderScale = XMVectorSet(scale.x, scale.y, 0.f, 0.f);
						float radius = collider->getRadius() * 2.f;
						colliderScale = XMVectorMultiply(XMVectorSet(radius, radius, 0.f, 0.f), colliderScale);

						XMVECTOR offset = XMVectorSet(collider->getOffset().x, collider->getOffset().y, 0.f, 0.f);
						offset = XMVector3Transform(offset, XMMatrixRotationQuaternion(rotation.get_dx()));

						colliderPosition += offset;

						debug_renderer_draw_ellipse(g_DebugBatch, vec3f(colliderPosition), vec2f(colliderScale), rotation, { 0u, 255u, 0u, 255u });
					}
				}
			}

			// Draw 2D Entity
			if (g_SelectedEntity != SV_ENTITY_NULL) {
				
				Transform trans = ecs_entity_transform_get(scene, g_SelectedEntity);

				XMMATRIX transformMatrix = trans.getWorldMatrix();

				XMVECTOR p0, p1, p2, p3;
				p0 = XMVector3Transform(XMVectorSet(-0.5f, -0.5f, 0.f, 0.f), transformMatrix);
				p1 = XMVector3Transform(XMVectorSet( 0.5f, -0.5f, 0.f, 0.f), transformMatrix);
				p2 = XMVector3Transform(XMVectorSet(-0.5f,  0.5f, 0.f, 0.f), transformMatrix);
				p3 = XMVector3Transform(XMVectorSet( 0.5f,  0.5f, 0.f, 0.f), transformMatrix);

				debug_renderer_linewidth_set(g_DebugBatch, 2.f);

				vec3f pos = trans.getWorldPosition();
				debug_renderer_draw_line(g_DebugBatch, { -float_max / 1000.f, pos.y, pos.z }, { float_max / 1000.f, pos.y, pos.z }, Color::Gray(100));
				debug_renderer_draw_line(g_DebugBatch, { pos.x, -float_max / 1000.f, pos.z }, { pos.x, float_max / 1000.f, pos.z }, Color::Gray(100));

				debug_renderer_linewidth_set(g_DebugBatch, 5.f);

				debug_renderer_draw_line(g_DebugBatch, vec3f(p0), vec3f(p1), Color::Orange());
				debug_renderer_draw_line(g_DebugBatch, vec3f(p0), vec3f(p2), Color::Orange());
				debug_renderer_draw_line(g_DebugBatch, vec3f(p2), vec3f(p3), Color::Orange());
				debug_renderer_draw_line(g_DebugBatch, vec3f(p1), vec3f(p3), Color::Orange());

				switch (g_TransformMode)
				{
				case sv::TransformMode_Translation:
				case sv::TransformMode_Scale:
				{
					vec3f scale = trans.getWorldScale();
					float arrowLength = std::max(scale.x, scale.y) * 0.6f;

					if (g_TransformMode == TransformMode_Translation && g_ActionMode == ActionMode_Transform) {
						auto& data = g_TransformModeData;

						debug_renderer_linewidth_set(g_DebugBatch, 4.f);
						debug_renderer_draw_line(g_DebugBatch, data.beginPos.get_vec3(), pos, Color::Gray(200));
					}

					Color xAxisColor = Color::Red();
					Color yAxisColor = Color::Green();

					if (g_ActionMode == ActionMode_Transform) {
						auto& data = g_TransformModeData;

						if (data.selectedXAxis) xAxisColor = Color::White();
						if (data.selectedYAxis) yAxisColor = Color::White();
					}

					debug_renderer_draw_line(g_DebugBatch, pos, pos + vec3f(arrowLength, 0.f, 0.f), xAxisColor);
					debug_renderer_draw_line(g_DebugBatch, pos, pos + vec3f(0.f, arrowLength, 0.f), yAxisColor);

					if (g_TransformMode == TransformMode_Scale) {

						debug_renderer_stroke_set(g_DebugBatch, 1.f);
						debug_renderer_draw_quad(g_DebugBatch, pos + vec3f(arrowLength, 0.f, 0.f), { arrowLength * 0.2f, arrowLength * 0.2f }, xAxisColor);
						debug_renderer_draw_quad(g_DebugBatch, pos + vec3f(0.f, arrowLength, 0.f), { arrowLength * 0.2f, arrowLength * 0.2f }, yAxisColor);
						
					}
					else {

					}
				}
				break;

				case sv::TransformMode_Rotation:

					float radius = trans.getWorldScale().get_vec2().length() / 2.f;

					constexpr ui32 CIRCLE_RESOLUTION = 180u;
					constexpr float adv = 2.f * PI / float(CIRCLE_RESOLUTION);

					float x0 = (cos(0.f) * radius) + pos.x;
					float y0 = (sin(0.f) * radius) + pos.y;
					float x1 = 0.f;
					float y1 = 0.f;

					for (float i = adv; i < 2.f * PI; i += adv) {

						x1 = (cos(i) * radius) + pos.x;
						y1 = (sin(i) * radius) + pos.y;

						debug_renderer_draw_line(g_DebugBatch, { x0, y0, pos.z }, { x1, y1, pos.z }, Color::White());

						x0 = x1;
						y0 = y1;

					}

					x1 = (cos(0.f) * radius) + pos.x;
					y1 = (sin(0.f) * radius) + pos.y;

					debug_renderer_draw_line(g_DebugBatch, { x0, y0, pos.z }, { x1, y1, pos.z }, Color::White());

					break;
				}
			}

			// Draw 2D grid
			if (g_Camera.camera.getProjectionType() == ProjectionType_Orthographic) {

				debug_renderer_linewidth_set(g_DebugBatch, 2.f);

				float width = g_Camera.camera.getWidth();
				float height = g_Camera.camera.getHeight();
				float mag = g_Camera.camera.getProjectionLength();

				ui32 count = 0u;
				for (float i = 0.01f; count < 3u ; i *= 10.f) {

					if (mag / i <= 50.f) {
						
						Color color;

						switch (count++)
						{
						case 0:
							color = Color::Gray(50);
							break;

						case 1:
							color = Color::Gray(100);
							break;

						case 2:
							color = Color::Gray(150);
							break;

						case 3:
							color = Color::Gray(200);
							break;
						}

						color.a = 10u;

						debug_renderer_linewidth_set(g_DebugBatch, count + 1.f);
						debug_renderer_draw_grid_orthographic(g_DebugBatch, g_Camera.position.get_vec2(), { width, height }, i, color);
					}
				}

				// Center
				float centerSize = (g_Camera.position.length() + mag) * 2.f;
				Color centerColor = Color::Gray(100);
				centerColor.a = 5u;
				debug_renderer_linewidth_set(g_DebugBatch, 2.f);
				debug_renderer_draw_line(g_DebugBatch, { -centerSize, -centerSize, 0.f }, { centerSize, centerSize, 0.f }, centerColor);
				debug_renderer_draw_line(g_DebugBatch, { -centerSize, centerSize, 0.f }, { centerSize, -centerSize, 0.f }, centerColor);
			}

			debug_renderer_batch_render(g_DebugBatch, g_Camera.camera.getOffscreenRT(), g_Camera.camera.getViewport(), g_Camera.camera.getScissor(), viewProjectionMatrix, cmd);
		}
	}

	DebugCamera& scene_editor_camera_get()
	{
		return g_Camera;
	}

	SceneEditorMode scene_editor_mode_get()
	{
		return g_EditorMode;
	}

	void scene_editor_mode_set(SceneEditorMode mode)
	{
		if (g_EditorMode == mode) return;

		g_EditorMode = mode;

		// Change camera

		switch (g_EditorMode)
		{
		case sv::SceneEditorMode_2D:
			g_Camera.camera.setProjectionType(ProjectionType_Orthographic);
			//g_Camera.settings.projection.cameraType = CameraType_Orthographic;
			//g_Camera.settings.projection.near = -100000.f;
			//g_Camera.settings.projection.far = 100000.f;
			//g_Camera.settings.projection.width = 20.f;
			//g_Camera.settings.projection.height = 20.f;
			g_Camera.position.z = 0.f;
			g_Camera.rotation = vec4f();
			break;
		case sv::SceneEditorMode_3D:
			g_Camera.camera.setProjectionType(ProjectionType_Perspective);
			//g_Camera.settings.projection.cameraType = CameraType_Perspective;
			//g_Camera.settings.projection.near = 0.01f;
			//g_Camera.settings.projection.far = 100000.f;
			//g_Camera.settings.projection.width = 0.01f;
			//g_Camera.settings.projection.height = 0.01f;
			g_Camera.position.z = -10.f;
			break;
		default:
			SV_LOG_INFO("Unknown editor mode");
			break;
		}
		
	}

	void scene_editor_selected_entity_set(Entity entity)
	{
		g_SelectedEntity = entity;
	}

	Entity scene_editor_selected_entity_get()
	{
		return g_SelectedEntity;
	}

}