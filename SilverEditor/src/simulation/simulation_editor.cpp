#include "core_editor.h"

#include "simulation_editor.h"
#include "simulation.h"
#include "platform/input.h"
#include "rendering/debug_renderer.h"

namespace sv {

	// Rendering 

	static RendererDebugBatch* g_DebugBatch;
	bool g_DebugRendering_Collisions = true;
	bool g_DebugRendering_Grid = true;

	// Action

	enum ActionMode {

		ActionMode_None,
		ActionMode_Transform,
		ActionMode_Selection,

	};

	static ActionMode g_ActionMode = ActionMode_None;
	static bool g_ActionStart = false;
	static struct {

		vec2f mousePos;

	} g_ActionStartData;

	// Transform Action
	TransformMode g_TransformMode = TransformMode_Translation;

	struct {

		vec2f beginMousePos;
		float beginMouseRotation;
		vec2f beginPos;
		vec2f beginScale;
		float beginRotation;
		bool selectedXAxis;
		bool selectedYAxis;
		bool selectedZAxis;

	} g_TransformModeData;

	///////////////////////////////////

	Result simulation_editor_initialize()
	{
		svCheck(debug_renderer_batch_create(&g_DebugBatch));

		return Result_Success;
	}

	Result simulation_editor_close()
	{
		svCheck(debug_renderer_batch_destroy(g_DebugBatch));

		return Result_Success;
	}

	void simulation_editor_camera_controller_2D(float dt)
	{
		static bool cameraInMovement = false;

		static struct {

			vec2f initialMovement;

		} cameraMovementData;

		auto& cam = g_DebugCamera.camera;
		float length = cam.getProjectionLength();

		// Movement
		if (input_mouse_pressed(SV_MOUSE_CENTER)) {
			cameraInMovement = true;

			cameraMovementData.initialMovement = g_SimulationMousePos;
		}
		else if (input_mouse_released(SV_MOUSE_CENTER)) {
			cameraInMovement = false;
		}

		if (cameraInMovement && g_SimulationMouseInCamera) {

			g_DebugCamera.position += (cameraMovementData.initialMovement - g_SimulationMousePos).getVec3();

		}

		// Zoom
		if (g_SimulationMouseInCamera) {
			length += -input_mouse_wheel_get() * length * 0.1f;
			if (length < 0.001f) length = 0.001f;

			cam.setProjectionLength(length);
		}

	}

	void simulation_editor_camera_controller_3D(float dt)
	{
		static bool cameraInMovement = false;

		static struct {

			vec2f initialMovement;

		} cameraMovementData;

		auto& cam = g_DebugCamera.camera;
		float length = cam.getProjectionLength();

		// Rotation
		{
			if (input_mouse(SV_MOUSE_RIGHT)) {
				vec2f drag = input_mouse_dragged_get();
				g_DebugCamera.rotation.x += drag.x;
				g_DebugCamera.rotation.y += drag.y;
			}
		}

		// Movement
		if (input_mouse_pressed(SV_MOUSE_CENTER)) {
			cameraInMovement = true;

			cameraMovementData.initialMovement = g_SimulationMousePos;
		}
		else if (input_mouse_released(SV_MOUSE_CENTER)) {
			cameraInMovement = false;
		}

		if (cameraInMovement && g_SimulationMouseInCamera) {

			XMVECTOR direction = (cameraMovementData.initialMovement - g_SimulationMousePos).get_dx();

			XMMATRIX rm = XMMatrixRotationQuaternion(g_DebugCamera.rotation.get_dx());

			direction = XMVector3Transform(direction, rm);

			g_DebugCamera.position += vec3f(direction);

		}

		// Zoom
		if (g_SimulationMouseInCamera) {
			length += -input_mouse_wheel_get() * length * 0.1f;
			if (length < 0.001f) length = 0.001f;

			cam.setProjectionLength(length);
		}

	}

	void simulation_editor_update_action(float dt)
	{
		Scene& scene = *g_Scene.get();

		// Transform mode
		if (input_key_pressed('T')) g_TransformMode = TransformMode_Translation;
		if (input_key_pressed('R')) g_TransformMode = TransformMode_Scale;
		if (input_key_pressed('E')) g_TransformMode = TransformMode_Rotation;

		if (g_SimulationMouseInCamera && input_mouse_pressed(SV_MOUSE_LEFT)) {
			g_ActionStart = true;
			g_ActionMode = ActionMode_None;

			g_ActionStartData.mousePos = g_SimulationMousePos;
		}

		if (g_ActionStart && g_SimulationMouseInCamera) {

			if (input_mouse_released(SV_MOUSE_LEFT)) {

				g_ActionStart = false;
				g_ActionMode = ActionMode_Selection;

			}

			else if (g_SelectedEntity != SV_ENTITY_NULL && input_mouse_dragged_get().length() != 0.f) {

				Transform trans = ecs_entity_transform_get(scene, g_SelectedEntity);

				vec3f pos = trans.getWorldPosition();
				vec3f scale = trans.getWorldScale();
				float rot = trans.getWorldEulerRotation().z;

				vec2f toMouse = g_ActionStartData.mousePos - pos.getVec2();

				g_TransformModeData.beginPos = pos.getVec2();
				g_TransformModeData.beginMousePos = g_ActionStartData.mousePos;
				g_TransformModeData.beginScale = scale.getVec2();
				g_TransformModeData.beginRotation = rot;
				g_TransformModeData.beginMouseRotation = toMouse.angle();

				float maxScale = std::max(abs(scale.x), abs(scale.y));

				if (g_TransformMode == TransformMode_Rotation) {

					float radius = vec2f(scale.x, scale.y).length() / 2.f;
					float distance = (toMouse).length();
					g_TransformModeData.selectedZAxis = distance > radius * 0.9f && distance < radius * 1.1f;

					g_TransformModeData.selectedXAxis = false;
					g_TransformModeData.selectedYAxis = false;
				}
				else {
					
					vec2f mousePos;
					if (input_key(SV_KEY_SHIFT)) {
						mousePos = toMouse;
						float a = toMouse.angle() - rot;
						mousePos.setAngle(a);
					}
					else {
						mousePos = toMouse;
					}

					BoundingBox2D xAxis;
					xAxis.init_minmax(vec2f{ 0.f, maxScale * -0.05f }, vec2f{ maxScale * 0.6f, maxScale * 0.05f });

					BoundingBox2D yAxis;
					yAxis.init_minmax(vec2f{ maxScale * -0.05f, 0.f }, vec2f{ maxScale * 0.05f, maxScale * 0.6f });

					g_TransformModeData.selectedXAxis = xAxis.intersects_point(mousePos);
					g_TransformModeData.selectedYAxis = yAxis.intersects_point(mousePos);
					g_TransformModeData.selectedZAxis = false;
				}

				if (g_TransformModeData.selectedXAxis || g_TransformModeData.selectedYAxis || g_TransformModeData.selectedZAxis || simulation_editor_is_entity_in_point(scene, g_SelectedEntity, g_ActionStartData.mousePos)) {
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
				vec2f pos = g_SimulationMousePos + diference;

				if (!data.selectedXAxis && !data.selectedYAxis) {
					trans.setPositionX(pos.x);
					trans.setPositionY(pos.y);
				}
				else if (input_key(SV_KEY_SHIFT)) {

					// TODO
					if (data.selectedXAxis) {
						trans.setPositionX(pos.x);
					}
					else if (data.selectedYAxis) {
						trans.setPositionY(pos.y);
					}

				}
				else {

					if (data.selectedXAxis) {
						trans.setPositionX(pos.x);
					}
					else if (data.selectedYAxis) {
						trans.setPositionY(pos.y);
					}

				}
			}
			break;
			case sv::TransformMode_Scale:
			{
				float dif = (g_SimulationMousePos - data.beginMousePos).length();
				if ((g_SimulationMousePos - data.beginPos).length() < (g_SimulationMousePos - data.beginMousePos).length()) dif = -dif;

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

				vec2f toMouse = g_SimulationMousePos - data.beginPos;
				vec3f eulerRotation = trans.getWorldEulerRotation();

				if (input_key(SV_KEY_SHIFT)) eulerRotation.z = toMouse.angle();
				else eulerRotation.z = data.beginRotation + toMouse.angle() - data.beginMouseRotation;
				
				trans.setEulerRotation(eulerRotation);

				break;
			}

			if (input_mouse_released(SV_MOUSE_LEFT)) {
				g_ActionMode = ActionMode_None;
			}
		}
		break;
		case sv::ActionMode_Selection:
		{
			std::vector<Entity> selectedEntities = simulation_editor_entities_in_point(g_SimulationMousePos);

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

	bool simulation_editor_is_entity_in_point(ECS* ecs, Entity entity, const vec2f& point)
	{
		Transform trans = ecs_entity_transform_get(ecs, entity);

		vec2f pos = trans.getWorldPosition().getVec2();
		float rot = trans.getWorldEulerRotation().z;
		vec2f auxMousePos = point - pos;
		rot -= auxMousePos.angle();
		auxMousePos.setAngle(rot);
		auxMousePos += pos;

		BoundingBox2D aabb;
		aabb.init_center(pos, trans.getWorldScale().getVec2());

		return aabb.intersects_point(auxMousePos);
	}

	std::vector<Entity> simulation_editor_entities_in_point(const vec2f& point)
	{
		ECS* ecs = g_Scene->getECS();
		std::vector<Entity> selectedEntities;

		ui32 entityCount = ecs_entity_count(ecs);

		for (ui32 i = 0; i < entityCount; ++i) {

			Entity entity = ecs_entity_get(ecs, i);

			if (simulation_editor_is_entity_in_point(ecs, entity, point)) {
				selectedEntities.push_back(entity);
			}
		}

		return selectedEntities;
	}

	void simulation_editor_render()
	{
		Scene& scene = *g_Scene.get();

		if (g_DebugCamera.camera.getProjectionType() == ProjectionType_Perspective)
			SceneRenderer::drawCamera3D(scene, &g_DebugCamera.camera, g_DebugCamera.position, g_DebugCamera.rotation);
		else
			SceneRenderer::drawCamera2D(scene, &g_DebugCamera.camera, g_DebugCamera.position, g_DebugCamera.rotation);

		// DEBUG RENDERING

		XMMATRIX viewProjectionMatrix = math_matrix_view(g_DebugCamera.position, g_DebugCamera.rotation) * g_DebugCamera.camera.getProjectionMatrix();
		CommandList cmd = graphics_commandlist_get();

		debug_renderer_batch_reset(g_DebugBatch);

		// Draw 2D Colliders
		if (g_DebugRendering_Collisions) {
			
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
		}

			// Draw 2D Entity
		if (g_SelectedEntity != SV_ENTITY_NULL) {

			Transform trans = ecs_entity_transform_get(scene, g_SelectedEntity);

			XMMATRIX transformMatrix = trans.getWorldMatrix();

			XMVECTOR p0, p1, p2, p3;
			p0 = XMVector3Transform(XMVectorSet(-0.5f, -0.5f, 0.f, 0.f), transformMatrix);
			p1 = XMVector3Transform(XMVectorSet(0.5f, -0.5f, 0.f, 0.f), transformMatrix);
			p2 = XMVector3Transform(XMVectorSet(-0.5f, 0.5f, 0.f, 0.f), transformMatrix);
			p3 = XMVector3Transform(XMVectorSet(0.5f, 0.5f, 0.f, 0.f), transformMatrix);

			debug_renderer_linewidth_set(g_DebugBatch, 2.f);

			vec3f pos = trans.getWorldPosition();
			vec3f scale = trans.getWorldScale();
			float maxScale = std::max(scale.x, scale.y);

			debug_renderer_draw_line(g_DebugBatch, { -float_max / 1000.f, pos.y, pos.z }, { pos.x - maxScale, pos.y, pos.z }, Color::Orange(50u));
			debug_renderer_draw_line(g_DebugBatch, { pos.x + maxScale, pos.y, pos.z }, { float_max / 1000.f, pos.y, pos.z }, Color::Orange(50u));

			debug_renderer_draw_line(g_DebugBatch, { pos.x, -float_max / 1000.f, pos.z }, { pos.x, pos.y - maxScale, pos.z }, Color::Orange(50u));
			debug_renderer_draw_line(g_DebugBatch, { pos.x, pos.y + maxScale, pos.z }, { pos.x, float_max / 1000.f, pos.z }, Color::Orange(50u));

			debug_renderer_linewidth_set(g_DebugBatch, 5.f);

			debug_renderer_draw_line(g_DebugBatch, vec3f(p0), vec3f(p1), Color::Orange());
			debug_renderer_draw_line(g_DebugBatch, vec3f(p0), vec3f(p2), Color::Orange());
			debug_renderer_draw_line(g_DebugBatch, vec3f(p2), vec3f(p3), Color::Orange());
			debug_renderer_draw_line(g_DebugBatch, vec3f(p1), vec3f(p3), Color::Orange());

			auto& data = g_TransformModeData;

			switch (g_TransformMode)
			{
			case sv::TransformMode_Translation:
			case sv::TransformMode_Scale:
			{
				float arrowLength = maxScale * 0.6f;

				if (g_TransformMode == TransformMode_Translation && g_ActionMode == ActionMode_Transform) {
					auto& data = g_TransformModeData;

					debug_renderer_linewidth_set(g_DebugBatch, 4.f);
					debug_renderer_draw_line(g_DebugBatch, data.beginPos.getVec3(), pos, Color::Gray(200));
				}

				Color xAxisColor = Color::Red();
				Color yAxisColor = Color::Green();

				float rot;

				if (input_key(SV_KEY_SHIFT)) rot = trans.getWorldEulerRotation().z;
				else rot = 0.f;

				vec2f xArrow = vec2f(arrowLength, 0.f);
				xArrow.setAngle(rot);

				vec2f yArrow = vec2f(arrowLength, 0.f);
				yArrow.setAngle(rot + (PI / 2.f));

				if (g_ActionMode == ActionMode_Transform) {
					if (data.selectedXAxis) xAxisColor = Color::White();
					if (data.selectedYAxis) yAxisColor = Color::White();
				}

				debug_renderer_draw_line(g_DebugBatch, pos, pos + xArrow.getVec3(), xAxisColor);
				debug_renderer_draw_line(g_DebugBatch, pos, pos + yArrow.getVec3(), yAxisColor);

				if (g_TransformMode == TransformMode_Scale) {

					debug_renderer_stroke_set(g_DebugBatch, 1.f);
					debug_renderer_draw_quad(g_DebugBatch, pos + xArrow.getVec3(), { arrowLength * 0.2f, arrowLength * 0.2f }, xAxisColor);
					debug_renderer_draw_quad(g_DebugBatch, pos + yArrow.getVec3(), { arrowLength * 0.2f, arrowLength * 0.2f }, yAxisColor);

				}
				else {

				}
			}
			break;

			case sv::TransformMode_Rotation:

				float radius = trans.getWorldScale().getVec2().length() / 2.f;
				vec2f toMouse = g_SimulationMousePos - data.beginPos;

				Color circumferenceColor = input_key(SV_KEY_SHIFT) ? Color{5u, 150u, 230u, 50u} : Color::Blue(50u);

				if (g_ActionMode == ActionMode_Transform) {

					circumferenceColor = Color::White();

				}
				else {

					float distance = (pos.getVec2() - g_SimulationMousePos).length();
					if (distance > radius * 0.9f && distance < radius * 1.1f) 
						circumferenceColor.a = 150u;

				}

				constexpr ui32 CIRCLE_RESOLUTION = 180u;
				constexpr float adv = 2.f * PI / float(CIRCLE_RESOLUTION);

				float x0 = (cos(0.f) * radius) + pos.x;
				float y0 = (sin(0.f) * radius) + pos.y;
				float x1 = 0.f;
				float y1 = 0.f;

				for (float i = adv; i < 2.f * PI; i += adv) {

					x1 = (cos(i) * radius) + pos.x;
					y1 = (sin(i) * radius) + pos.y;

					debug_renderer_draw_line(g_DebugBatch, { x0, y0, pos.z }, { x1, y1, pos.z }, circumferenceColor);

					x0 = x1;
					y0 = y1;

				}

				x1 = (cos(0.f) * radius) + pos.x;
				y1 = (sin(0.f) * radius) + pos.y;

				debug_renderer_draw_line(g_DebugBatch, { x0, y0, pos.z }, { x1, y1, pos.z }, circumferenceColor);

				if (g_ActionMode == ActionMode_Transform) {

					vec2f beginRotationVector(radius, 0.f);
					vec2f currentRotationVector(radius, 0.f);

					if (input_key(SV_KEY_SHIFT)) {
						beginRotationVector.setAngle(data.beginRotation);
						currentRotationVector.setAngle(toMouse.angle());
					}
					else {
						beginRotationVector.setAngle(data.beginMouseRotation);
						currentRotationVector.setAngle(toMouse.angle());
					}

					debug_renderer_draw_line(g_DebugBatch, pos, pos + beginRotationVector.getVec3(), Color::White());
					debug_renderer_draw_line(g_DebugBatch, pos, pos + currentRotationVector.getVec3(), Color::White());
				}

				break;
			}
		}

		// Draw 2D grid
		if (g_DebugRendering_Grid && g_DebugCamera.camera.getProjectionType() == ProjectionType_Orthographic) {

			debug_renderer_linewidth_set(g_DebugBatch, 2.f);

			float width = g_DebugCamera.camera.getWidth();
			float height = g_DebugCamera.camera.getHeight();
			float mag = g_DebugCamera.camera.getProjectionLength();

			ui32 count = 0u;
			for (float i = 0.01f; count < 3u; i *= 10.f) {

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
					debug_renderer_draw_grid_orthographic(g_DebugBatch, g_DebugCamera.position.getVec2(), { width, height }, i, color);
				}
			}

			// Center
			float centerSize = (g_DebugCamera.position.length() + mag) * 2.f;
			Color centerColor = Color::Gray(100);
			centerColor.a = 5u;
			debug_renderer_linewidth_set(g_DebugBatch, 2.f);
			debug_renderer_draw_line(g_DebugBatch, { -centerSize, -centerSize, 0.f }, { centerSize, centerSize, 0.f }, centerColor);
			debug_renderer_draw_line(g_DebugBatch, { -centerSize, centerSize, 0.f }, { centerSize, -centerSize, 0.f }, centerColor);
		}

		debug_renderer_batch_render(g_DebugBatch, g_DebugCamera.camera.getOffscreenRT(), g_DebugCamera.camera.getViewport(), g_DebugCamera.camera.getScissor(), viewProjectionMatrix, cmd);
	}

}